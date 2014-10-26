#include <iostream>
#include <functional>
#include <vector>
#include <thread>
#include <cerrno>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h> // for memset

#include "ChatManager.hpp"
#include "ClientHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::thread;

using ChatServer::ClientHandler;
using ChatServer::ChatManager;

void start_processing(int fd, ChatManager& cm)
{
	ClientHandler ch(fd, cm);
	ch.HandleClient();
}

int main(int argc, char** argv)
{
	int server_sock_fd, client_sock_fd, port_number;
	socklen_t client_length;
	struct sockaddr_in server_address, client_address;
	int result = 0;
	int pid = 0;

	// Create the ChatManager object
	ChatManager cm;

	// Vector of threads for handling clients
	vector<thread> threads;

	// Vector of client handlers
	vector<ClientHandler> handlers;

	// Create a socket
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
	int yes = 1;
	result = setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if(result != 0)
	{
		cerr << "Error: could not set socket option (errno: " << errno << ")" << endl;
		return -1;
	}

	// Bind
	result = bind(server_sock_fd, (struct sockaddr*)&server_address, sizeof(server_address));
	if(result != 0)
	{
		cerr << "Error: could not bind socket (errno: " << errno << ")" << endl;
		return -1;
	}
	
	// Listen for connections
	result = listen(server_sock_fd, 10);
	if(result != 0)
	{
		cerr << "Error: could not listen (errno: " << errno << ")" << endl;
		close(server_sock_fd);
		return -1;
	}

	cout << "Server established, listening on port " << port_number << "..." << endl;
	try
	{
		while(true)
		{
			// Accept the connection, spawn a new thread to handle it
			client_length = sizeof(client_address);
			client_sock_fd = accept(server_sock_fd, (struct sockaddr*)&client_address, &client_length);

			if(client_sock_fd < 0)
			{
				cerr << "Error: could not accept client (errno: " << errno << ")" << endl;
				close(client_sock_fd);
				continue;
			}

			thread t(start_processing, client_sock_fd, std::ref(cm));
			t.detach();
		}
	} 
	catch(const std::runtime_error& e)
	{
		cerr << "Error accepting connections!" << std::endl;
		cerr << e.what() << endl;
	}
	catch(...)
	{
		cerr << "NAMELESS EVIL!" << endl;
	}

	// Clean up the socket
	close(server_sock_fd);

	return 0;
}

