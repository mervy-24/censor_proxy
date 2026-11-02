
#include "common.h"

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
		return EXIT_FAILURE;
	}


	if (listen(listen_fd, 5) < 0) {
		perror("listen");
		return EXIT_FAILURE;
	}

	// Define the client address:
	struct sockaddr_in client_address;
	socklen_t client_len = sizeof(client_address);
	int client_fd = accept(listen_fd, (struct sockaddr *)&client_address, &client_len);
	if (client_fd < 0) {
		perror("accept");
		close(listen_fd);
		return EXIT_FAILURE;
	}

	// Convery client IP binary to string:
	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));
	int client_port = ntohs(client_address.sin_port);

	char buffer[1024];
	ssize_t n;

	// Echo loop (loop body only runs if n > 0):
	// n > 0: received bytes; n = 0: client closed connection; n < 0: error.
	while ((n = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
		// Print what was received:
		printf("Received %zd bytes from \'%s:%d:\' ", n, client_ip, client_port);
		fwrite(buffer, 1, n, stdout);
		fflush(stdout);

		ssize_t total = 0;

		// Echoing back:
		while (total < n) {
			ssize_t sent = send(client_fd, buffer + total, n - total, 0);
			if (sent < 0) {
				perror("send");
				goto done;
			}
			total += sent;
		}
	}

	if (n < 0) {
		perror("recv");
	}

	if (n == 0) {
		printf("Client from \'%s:%d\' disconnected.\n", client_ip, client_port);
	}

	// Close the sockets:
	done:
		close(client_fd);
		close(listen_fd);
		return 0;

}

