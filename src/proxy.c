
#include "common.h"
#include <poll.h>

#define MAX_CLIENTS 64

int main(int argc, char *argv[]) {

	// Error check:
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

	// Create the server socket:
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
	memset(&server_address, 0, sizeof(server_address)); // Zeroing the address struct.
	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_port = htons(port); // Port from argument.
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
	
	// Bind the socket to specified IP and Port:
	if (bind(listen_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
		perror("bind");
		close(listen_fd);
		return EXIT_FAILURE;
	}


	if (listen(listen_fd, 5) < 0) {
		perror("listen");
		close(listen_fd);
		return EXIT_FAILURE;
	}

	struct pollfd fds[64];
	char ips[64][INET_ADDRSTRLEN];
	int ports[64];
	int nfds = 1;

	fds[0].fd = listen_fd; // Listening socket.
	fds[0].events = POLLIN; // Client waiting to be accepted.

	printf("Proxy listening on port %d\n", port);

	// Sleep until socket is ready:
	while (1) {
		int ready = poll(fds, nfds, -1);
		if (ready < 0) {
			perror("poll");
			break;
		}

		if (fds[0].revents & POLLIN) {
			// Define the client address:
			struct sockaddr_in client_address;
			socklen_t client_len = sizeof(client_address);

			int client_fd = accept(listen_fd, (struct sockaddr *)&client_address, &client_len);
			if (client_fd < 0) {
				perror("accept");
			} else if (nfds >= 64) {
				fprintf(stderr, "Too many clients, rejecting.\n");
				close(client_fd);
			} else {
				fds[nfds].fd = client_fd;
				fds[nfds].events = POLLIN;

				inet_ntop(AF_INET, &client_address.sin_addr, ips[nfds], sizeof(ips[nfds]));
				ports[nfds] = ntohs(client_address.sin_port);
				printf("Client %s:%d connected.\n", ips[nfds], ports[nfds]);

				nfds++;
			}
		}

		// Echo loop:
		for (int i = 1; i < nfds; i++) {
			if (!(fds[i].revents & (POLLIN | POLLHUP || POLLERR))) { continue; } // Check dead connection.

			char buffer[1024];
			ssize_t n = recv(fds[i].fd, buffer, sizeof(buffer), 0);
			int drop = 0;

			if (n > 0) {
				printf("Received %zd bytes from Client %s:%d: ", n, ips[i], ports[i]);
				fwrite(buffer, 1, n, stdout);
				fflush(stdout);

				ssize_t total = 0;
				while (total < n) {
					ssize_t sent = send(fds[i].fd, buffer + total, n - total, 0);
					if (sent < 0) {
						perror("send");
						drop = 1;
						break;
					}
					total += sent;
				}
			} else if (n == 0) {
				printf("Client %s:%d disconnected.\n", ips[i], ports[i]);
				drop = 1;
			} else {
				perror("recv");
				drop = 1;
			}

			if (drop) {
				close(fds[i].fd);

				nfds--;
				fds[i] = fds[nfds];
				strcpy(ips[i], ips[nfds]);
				ports[i] = ports[nfds];

				i--;
			}
		}
	}

	close(listen_fd);
	return 0;

}

