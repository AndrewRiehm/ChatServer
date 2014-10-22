#include <iostream>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h> // for memset

#include "ClientHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char** argv)
{
	int server_sock_fd, client_sock_fd, port_number;
	socklen_t client_length;
	struct sockaddr_in server_address, client_address;
	int result = 0;
	int pid = 0;

	// Create a socket
	cout << "Creating a socket FD..." << endl;
	server_sock_fd = socket(PF_INET, SOCK_STREAM, 0);

	if(server_sock_fd < 0)
	{
		cerr << "Error: could not create socket!" << endl;
		return -1;
	}

	// Initialize address structure
	memset((char*)&server_address, '\0', sizeof(server_address));
	port_number = 4919; // 0x1337
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port_number);

	// Make sure the port is not in use.
	cout << "Setting socket options..." << endl;
	int yes = 1;
	result = setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if(result != 0)
	{
		cerr << "Error: could not set socket option (errno: " << errno << ")" << endl;
		return -1;
	}

	// Bind
	cout << "Binding..." << endl;
	result = bind(server_sock_fd, (struct sockaddr*)&server_address, sizeof(server_address));
	if(result != 0)
	{
		cerr << "Error: could not bind socket (errno: " << errno << ")" << endl;
		return -1;
	}
	

	// Listen for connections
	cout << "Listening..." << endl;
	result = listen(server_sock_fd, 10);
	if(result != 0)
	{
		cerr << "Error: could not listen (errno: " << errno << ")" << endl;
		close(server_sock_fd);
		return -1;
	}

	try
	{
		while(true)
		{
			// Accept the connection, fork a new process to respond
			client_length = sizeof(client_address);
			client_sock_fd = accept(server_sock_fd, (struct sockaddr*)&client_address, &client_length);

			if(client_sock_fd < 0)
			{
				cerr << "Error: could not accept client (errno: " << errno << ")" << endl;
				close(client_sock_fd);
				continue;
			}
			cout << "Got a new client!" << endl;

			pid = fork();
			if(pid == 0)
			{ 
				// We're the child - close the server socket, we shouldn't be using it!
				close(server_sock_fd);
				
				// Handle the client
				ChatServer::ClientHandler handler(client_sock_fd);
				handler.HandleClient();

				// Nothing more to do - terminate client process!
				return 0;
			}
			else
			{
				cout << "Spawned child process (pid: " << pid << ")" << endl;
				// We're the parent - close the client socket, we shouldn't use it!
				close(client_sock_fd);
			}
		}
	} 
	catch(std::exception& e)
	{
		cerr << "Error accepting connections!" << std::endl;
	}

	// Clean up the socket
	close(server_sock_fd);

	return 0;
}
