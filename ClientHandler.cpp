
#include "ClientHandler.hpp"
#include "ChatManager.hpp"
#include "Command.hpp"

#include <iostream>
#include <functional>
#include <unistd.h>
#include <string>
#include <string.h> // for memset

using std::cout;
using std::endl;
using std::cerr;
using std::string;

using ChatServer::Command;

ChatServer::ClientHandler::ClientHandler(int fd, ChatManager& cm): _iSocketFD(fd), _cm(cm)
{
	//-------------------------------------------------------
	// Set up the command objects
	//-------------------------------------------------------
	Command quit;
	quit.strString = "/quit";
	quit.strDescription = "Disconnects from the chat server.  Not for winners.";
	quit.Execute = std::bind(&ClientHandler::QuitHandler, this, std::placeholders::_1);
	_mCommands[quit.strString] = quit;
}

ChatServer::ClientHandler::~ClientHandler()
{
	close(_iSocketFD);
}

void ChatServer::ClientHandler::HandleClient()
{
	cout << "Handling client on " << _iSocketFD << endl;
	int result = 0;

	// Make them login first
	LoginHandler("");

	while(true)
	{
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
				cerr << "Error: invalid command received from (socket: " << _iSocketFD 
						 << ", name: " << _strUserName << "): " << msg << endl;
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
	}

	close(_iSocketFD);
}

//---------------------------------------------------------
// Command handler functions
//---------------------------------------------------------
void ChatServer::ClientHandler::LoginHandler(std::string args)
{
	cout << "Handling a login! args: " << args << endl;
	WriteString("Welcome to the XYZ chat server!\n");

	_strUserName = "";
	int tries_left = 5;
	do
	{
		WriteString("Login Name?\n");
		_strUserName = ReadString();

		// Check for name collision
		if(!_cm.AddClient(this))
		{
			WriteString("Sorry, name taken.\n");

			// Try again, name was taken
			_strUserName = "";
		}
	} while(_strUserName == "" && --tries_left > 0);

	if(tries_left <= 0)
	{
		WriteString("Max number of attempts reached.  No soup for you!  Come back one year!\n");
		Bail("too many invalid login attempts");
	}

	WriteString("Welcome, " + _strUserName + "\n");
  ListCommands();
}

void ChatServer::ClientHandler::QuitHandler(std::string args)
{
	_cm.RemoveClient(this);
	WriteString("BYE\n");
	close(_iSocketFD);
	exit(EXIT_SUCCESS);
}

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------

void ChatServer::ClientHandler::ListCommands()
{
	string msg = "Available commands:\n";
	for(auto& item: _mCommands)
	{
		msg += "\t" + item.second.strString + ": " + item.second.strDescription + "\n";
	}
	WriteString(msg);
}

void ChatServer::ClientHandler::SendMsg(const std::string& msg)
{
	// TODO: Wrap this message with any decoration before sending?
	WriteString(msg);
}

std::string ChatServer::ClientHandler::ReadString()
{
	const int BUF_SIZE = 512;
	int bytesRead = 0;
	char buffer[BUF_SIZE];
	memset(buffer, '\0', BUF_SIZE);

	// Read the message from the client
	bytesRead = read(_iSocketFD, buffer, BUF_SIZE-1);
	buffer[bytesRead] = '\0'; // Forcibly null-terminate the char buffer
	if(bytesRead < 0)
	{
		Bail("could not read from client socket");
	}

	return Scrub(buffer, bytesRead);
}

void ChatServer::ClientHandler::WriteString(const std::string& msg)
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
    else
    {
      msg += ' ';
    }
	}
	return msg;
}

//---------------------------------------------------------
// Getters & setters
//---------------------------------------------------------
std::string ChatServer::ClientHandler::GetUserName() const
{
	return _strUserName;
}

std::string ChatServer::ClientHandler::GetCurrentRoom() const
{
	return _strCurrentRoom;
}
