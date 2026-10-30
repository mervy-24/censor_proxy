#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

int main() {
	
	// Create socket:
	int client_fd; // Holds socket info.
	client_fd = socket(AF_INET, SOCK_STREAM, 0);

	// Specify an address for socket:
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_port = htons(9090); // Port 9090
	server_address.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
	
	int connection_status = connect(client_fd, (struct sockaddr *) &server_address, sizeof(server_address));

	if (connection_status == -1) {
	       printf("There was an error making a connection to the server.\n");
	}

	// Receive data from the server:
	char server_response[256];
	recv(client_fd, &server_response, sizeof(server_response), 0);

	// Print out the server's response:
	printf("The server sent the data: %s\n", server_response);

	// Close the socket:
	close(client_fd);

	return 0;
}
