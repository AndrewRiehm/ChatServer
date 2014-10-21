
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
	int result = write(_iSocketFD, msg.c_str(), sizeof(char)*msg.length());
	if(result < 0)
	{
		cerr << "Error: could not read message from client (errno: " << errno << ")" << endl;
	}
}

void ChatServer::ClientHandler::HandleClient()
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
		return;
	}

	string msg(buffer);
	msg.erase(msg.length()-2, 2); // Chop off \r\n  TODO: Better way to validate commands
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
			cerr << "Error: could not write message to client (errno: " << errno << ")" << endl;
			close(_iSocketFD);
		}
	}



	close(_iSocketFD);
}

void ChatServer::ClientHandler::LoginHandler(std::string args)
{
	cout << "Handling a login! args: " << args << endl;
	string msg = "Gotta login, bro:\n";
	int result = write(_iSocketFD, msg.c_str(), msg.length());
	if(result < 0)
	{
		cerr << "Error: could not write message to client (errno: " << errno << ")" << endl;
	}
}
