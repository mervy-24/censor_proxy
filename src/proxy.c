
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

	char server_message [256] = "You have reached the server!";

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

	int client_fd = accept(listen_fd, NULL, NULL);
	if (client_fd < 0) {
		perror("accept");
		return EXIT_FAILURE;
	}
	
	// Send the message:
	if (send(client_fd, server_message, sizeof(server_message), 0) < 0) {
		perror("send");
		return EXIT_FAILURE;
	}

	// Close the socket:
	close(client_fd);
	close(listen_fd);
	
	return 0;
}

