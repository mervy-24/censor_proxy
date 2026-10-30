
#include "common.h"

int main() {

	char server_message [256] = "You have reached the server!";

	// Create the server socket:
	int server_fd;
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// Define the server address:
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_port = htons(DEFAULT_PORT); // Port 9090
	server_address.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
	
	// Bind the socket to specified IP and Port:
	bind(server_fd, (struct sockaddr*) &server_address, sizeof(server_address));

	listen(server_fd, 5);

	int client_fd;
	client_fd = accept(server_fd, NULL, NULL);
	
	// Send the message:
	send(client_fd, server_message, sizeof(server_message), 0);

	// Close the socket:
	close(server_fd);
	
	return 0;
}


