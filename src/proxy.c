
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include "common.h"

#define BUF_SIZE 1024
#define DEFAULT_PORT 9090
#define MAX_CLIENTS 64
#define LINE_MAX_LEN 4096


// Keep calling send until buffer completed, rather than partial:
static int send_all(int fd, const char *data, size_t len) {
	size_t total = 0;
	
	while (total < len) {
		ssize_t sent = send(fd, data + total, len - total, MSG_NOSIGNAL);

		if (sent < 0) {
			perror("send");
			return -1;
		}
		total += sent;
	}
	return 0;
}

// Next client slots wraps around client i's line:
static int pick_target(int i, int nfds) {
	if (nfds <= 2) return i; // Listener + one client.
	int t = i + 1;
	if (t >= nfds) t = 1; // Wrap past the last client back to slot 1.
	return t;
}

int main(int argc, char *argv[]) {

	// Argument error check:
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Parse and validate port:
	int port = atoi(argv[1]);

	if (port <= 0 || port > 65535) {
		fprintf(stderr, "Invalid port: %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	// Create the server socket (IPv4, TCP):
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (listen_fd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	// Allow quick restarts without "Address already in use":
	int opt = 1;

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			perror("setsockopt");
			close(listen_fd);
			return EXIT_FAILURE;
	}

	// Define the server address:
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address)); 	// Zeroing the struct.
	server_address.sin_family = AF_INET; 			// IPv4
	server_address.sin_port = htons(port); 			// Port from argument.
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); 	// 0.0.0.0
	
	// Bind the socket to specified IP and Port:
	if (bind(listen_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
		perror("bind");
		close(listen_fd);
		return EXIT_FAILURE;
	}

	// Mark the socket as passive:
	if (listen(listen_fd, 5) < 0) {
		perror("listen");
		close(listen_fd);
		return EXIT_FAILURE;
	}

	// Load the wordlist:
	wordlist_t wl;
	int loaded = wordlist_load("data/wordlist.txt", &wl);

	if (loaded < 0) {
		close(listen_fd);
		return EXIT_FAILURE;
	}
	printf("Loaded %d words from data/wordlist,txt\n", loaded);

	// Per client stated:
	struct pollfd fds[MAX_CLIENTS + 1];
	char ips[MAX_CLIENTS + 1][INET_ADDRSTRLEN];
	int ports[MAX_CLIENTS + 1];
	char linebuf[MAX_CLIENTS + 1][LINE_MAX_LEN]; 	// Bytes received but not yet one full line.
	size_t linelen[MAX_CLIENTS + 1]; 		// How many bytes are in linebuf[i].
	char out[3 * LINE_MAX_LEN];			// Censored output.
	int nfds = 1;					// Number of slots in use (default 1 = listener.
	fds[0].fd = listen_fd; 				// Listening socket.
	fds[0].events = POLLIN; 			// Client waiting to be accepted.
	printf("Proxy listening on port %d\n", port);

	// Main event loop: sleep until socket is ready:
	while (1) {

		// Block until at least one socket is ready:
		int ready = poll(fds, nfds, -1);

		if (ready < 0) {
			perror("poll");
			break;
		}

		// New connection on the listener:
		if (fds[0].revents & POLLIN) {

			// Define the client address on the listener:
			struct sockaddr_in client_address;
			socklen_t client_len = sizeof(client_address);
			int client_fd = accept(listen_fd, (struct sockaddr *)&client_address, &client_len);

			if (client_fd < 0) {
				perror("accept");

			// No free slots in nfds:
			} else if (nfds >= MAX_CLIENTS + 1) {
				fprintf(stderr, "Too many clients, rejecting.\n");
				close(client_fd);

			} else {

				// Store new client in the first free slot (i nfds):
				fds[nfds].fd = client_fd;
				fds[nfds].events = POLLIN; 	// Event: client sends data.
				inet_ntop(AF_INET, &client_address.sin_addr, ips[nfds], sizeof(ips[nfds])); // Converts binary IP -> readable IP.
				ports[nfds] = ntohs(client_address.sin_port);
				linelen[nfds] = 0; 		// Empties line buffer.
				printf("Client %s:%d connected.\n", ips[nfds], ports[nfds]);
				nfds++; 			// Poll watches new slot.
			}
		}

		// Check every client for data:
		for (int i = 1; i < nfds; i++) {

			if (!(fds[i].revents & (POLLIN | POLLHUP | POLLERR))) continue; // Check dead connection.
			char buffer[1024];
			ssize_t n = recv(fds[i].fd, buffer, sizeof(buffer), 0);
			int drop = 0;

			// n > 0: bytes received; n == 0: client closed; n < 0: error:
			if (n > 0) {
				size_t got = (size_t)n;

				// Catches a client that never sends \n:
				if (got > LINE_MAX_LEN - linelen[i]) {
					fprintf(stderr, "Client %s:%d sent a line that is too long, dropping.\n", ips[i], ports[i]);
					drop = 1;
				} else {

					// Append new bytes:
					memcpy(linebuf[i] + linelen[i], buffer, n);
					linelen[i] += n;

					// Extract every complete line:
					char *nl;

					while ((nl = memchr(linebuf[i], '\n', linelen[i])) != NULL) {
						size_t line_len = (nl - linebuf[i]) + 1;
						
						// Censor into out:
						size_t out_len = censor_line(linebuf[i], line_len, out, sizeof(out), &wl);
						int target = pick_target(i, nfds);
						printf("REceived %zu bytes from Client %s:%d: ", line_len, ips[i], ports[i]);
						fwrite(linebuf[i], 1, line_len, stdout); // Original uncensored line.
						printf("	-> %zu bytes to Client %s:%d:\n", out_len, ips[target], ports[target]);
						fflush(stdout);

						// Send the censored bytes:
						if (send_all(fds[target].fd, out, out_len) < 0 && target == i) {
							drop = 1;
							break;
						}
						
						// Shift leftover bytes to the front:
						memmove(linebuf[i], linebuf[i] + line_len, linelen[i] - line_len);
						linelen[i] -= line_len;
					}

				}	
			
			} else if (n == 0) {
					printf("Client %s:%d disconnected.\n", ips[i], ports[i]);
					drop = 1;
				
			} else {
				perror("recv");
				drop = 1;
			}
		
			// Remove the client if drop == 1:
			if (drop) {
				close(fds[i].fd);
				nfds--;
				fds[i] = fds[nfds];
				strcpy(ips[i], ips[nfds]);
				ports[i] = ports[nfds];
				memcpy(linebuf[i], linebuf[nfds], linelen[nfds]); // Only copy used bytes.
				linelen[i] = linelen[nfds];
				i--;
			}
		}
	}
	wordlist_free(&wl);
	close(listen_fd);
	return 0;
}

