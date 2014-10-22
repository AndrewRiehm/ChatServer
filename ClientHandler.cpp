
#include "ClientHandler.hpp"
#include "Command.hpp"

#include <iostream>
#include <unistd.h>
#include <string>

using std::cout;
using std::endl;
using std::cerr;
using std::string;

using ChatServer::Command;

ChatServer::ClientHandler::ClientHandler(int fd)
{
	_iSocketFD = fd;

	
	Command login;
	login.strString = "/login";
	login.strDescription = "Prompts the user for a login name.";
	login.Execute = std::bind(&ClientHandler::LoginHandler, this, std::placeholders::_1);
	_mCommands[login.strString] = login;
}

ChatServer::ClientHandler::~ClientHandler()
{
	close(_iSocketFD);
}

void ChatServer::ClientHandler::ListCommands()
{
	string msg = "Available commands:\n";
	for(auto& item: _mCommands)
	{
		msg += "\t" + item.second.strString + ": " + item.second.strDescription + "\n";
	}
}

void ChatServer::ClientHandler::HandleClient()
{
	cout << "Handling client on " << _iSocketFD << endl;
	int result = 0;

	// Read the message from the client
	string msg = ReadString(); 

	// Check to see if we've got a command or a generic chat message
	if(msg[0] == '/') 
	{
		// Looks like the user wants to issue a command, see if it's valid
		if(_mCommands.find(msg) != _mCommands.end())
		{
			// Found a valid command - execute it!
			_mCommands[msg].Execute(msg);
		}
		else
		{
			// Not a valid command
			cerr << "Error: invalid command received from client " << _iSocketFD 
					 << ": " << msg << endl;
			ListCommands();
		}
	}
	else
	{
		// Got a chat message
		// TODO: If they're in a room, send the message to that room
		//       ELSE print an error suggesting they select or create a room
		cout << "Chat message: " << msg << endl;
		result = write(_iSocketFD, "Got it!\n", 8);
		if(result < 0)
		{
			Bail("could not write to client socket");
		}
	}

	close(_iSocketFD);
}

void ChatServer::ClientHandler::LoginHandler(std::string args)
{
	int BUF_SIZE = 256;
	char buffer[BUF_SIZE];
	bzero(buffer, BUF_SIZE);

	cout << "Handling a login! args: " << args << endl;
	string msg = "Welcome to the XYZ chat server!\nLogin Name?\n";
	int result = write(_iSocketFD, msg.c_str(), msg.length());
	if(result < 0)
	{
		cerr << "Error: could not write message to client (errno: " << errno << ")" << endl;
	}
}

std::string ChatServer::ClientHandler::ReadString()
{
	const int BUF_SIZE = 512;
	int bytesRead = 0;
	char buffer[BUF_SIZE];
	bzero(buffer, BUF_SIZE);

	// Read the message from the client
	bytesRead = read(_iSocketFD, buffer, BUF_SIZE-1);
	buffer[bytesRead] = '\0'; // Forcibly null-terminate the char buffer
	if(bytesRead < 0)
	{
		Bail("could not read from client socket");
	}

	return Scrub(buffer, bytesRead);
}

void ChatServer::ClientHandler::WriteString(std::string msg)
{
	int result = write(_iSocketFD, msg.c_str(), sizeof(char)*msg.length());
	if(result < 0)
	{
		Bail("could not read from client socket");
	}
}

void ChatServer::ClientHandler::Bail(std::string err)
{
	cerr << "Error: " << err << " (errno: " << errno << ")" << endl;
	close(_iSocketFD);
	exit(EXIT_FAILURE);
}

std::string ChatServer::ClientHandler::Scrub(const char* buf, const int buf_size)
{
	// Make sure every character is ascii, and it ends with null terminator
	// int of 32-126 inclusive
	string msg = "";
	for(int i = 0; i < buf_size; ++i)
	{
		if(buf[i] == '\0' || buf[i] == '\n' || buf[i] == '\r')
		{
			// Found a null-terminator - this should be the end of the string
			// Also, assume that \n or \r signal the end of a message
			return msg;
		}

		if((buf[i] >= 32 && buf[i] <= 126)) 
		{
			msg += buf[i];
		}
	}
	return msg;
}
