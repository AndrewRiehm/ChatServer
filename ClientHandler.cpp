
#include "ClientHandler.hpp"

#include <iostream>
#include <unistd.h>

using std::cout;
using std::endl;
using std::cerr;

ClientHandler::ClientHandler(int fd)
{
	_iSocketFD = fd;
}

ClientHandler::~ClientHandler()
{
	close(_iSocketFD);
}

void ClientHandler::HandleClient()
{
	cout << "Handling client on " << _iSocketFD << endl;
	const int BUF_SIZE = 256;
	int result = 0;
	char buffer[BUF_SIZE];
	bzero(buffer, BUF_SIZE);

	// Read the message from the client
	result = read(_iSocketFD, buffer, BUF_SIZE-1);
	if(result < 0)
	{
		cerr << "Error: could not read message from client (errno: " << errno << ")" << endl;
		close(_iSocketFD);
	}
	else
	{
		cout << "Got a message: " << buffer << endl;
	}

	result = write(_iSocketFD, "Got it!\n", 8);
	if(result < 0)
	{
		cerr << "Error: could not read message from client (errno: " << errno << ")" << endl;
		close(_iSocketFD);
	}

	close(_iSocketFD);
}
