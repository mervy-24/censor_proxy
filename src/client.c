
#include "common.h"

int main(int argc, char *argv[]) {
	
	// Argument error check:
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <proxy-ip> <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Parse and validate port:
	int port = atoi(argv[2]);
	if (port <= 0 || port > 65535) {
		fprintf(stderr, "Invalid port: %s\n", argv[2]);
		return EXIT_FAILURE;
	}

	// Create socket:
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	// Specify an address for socket:
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address)); // Zeroing the address struct.
	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_port = htons(port); // Port from argument.
	if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) != 1) {
		fprintf(stderr, "Invalid IP address\n");
		close(client_fd);
		return EXIT_FAILURE;
	}

	// Connect to proxy:	
	if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
		perror("connect");
		close(client_fd);
		return EXIT_FAILURE;
	}

	char line[1024];
	char reply[1024];

	// Main echo loop:
	while (fgets(line, sizeof(line), stdin) != NULL) {
		size_t len = strlen(line);

		// Send the whole line (handle partial sends):
		size_t sent_total = 0;
		while (sent_total < len) {
			ssize_t sent = send(client_fd, line + sent_total, len - sent_total, 0);
			if (sent < 0) {
				perror("send");
				close(client_fd);
				return EXIT_FAILURE;
			}
			sent_total += sent;
		}

		// Receive the echo (expect exactly len bytes back:
		size_t recv_total = 0;
		while (recv_total < len) {
			ssize_t n = recv(client_fd, reply + recv_total, len - recv_total, 0);
			if (n < 0) {
				perror("recv");
				close(client_fd);
				return EXIT_FAILURE;
			}
			if (n == 0) {
				fprintf(stderr, "Proxy closed the connection.\n");
				close(client_fd);
				return EXIT_FAILURE;
			}
			recv_total += n;
		}

		reply[recv_total] = '\0';
		printf("Echo: %s", reply);
	}

	// Close the socket:
	close(client_fd);
	return 0;

}
