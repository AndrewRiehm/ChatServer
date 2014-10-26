
#include "ClientHandler.hpp"
#include "ChatManager.hpp"
#include "Command.hpp"

#include <iostream>
#include <sstream>
#include <functional>
#include <unistd.h>
#include <string>
#include <string.h> // for memset
#include <stdexcept>
#include <thread>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <syslog.h> // syslog!

using std::endl;
using std::string;
using std::chrono::seconds;
using std::chrono::duration_cast;
using std::chrono::steady_clock;

using ChatServer::Command;

ChatServer::ClientHandler::ClientHandler(int fd, ChatManager& cm)
	: _iSocketFD(fd), _cm(cm), _bDone(false)
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	// Make sure we ignore SIGPIPE (writing to closed connection)
	// Only works on BSD-based systems
	int set = 1;
	setsockopt(_iSocketFD, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif

	//-------------------------------------------------------
	// Set up the command objects
	//-------------------------------------------------------
	Command quit;
	quit.strString = "/quit";
	quit.strDescription = "Disconnects from the chat server.  Not for winners.";
	quit.Execute = std::bind(&ClientHandler::QuitHandler, this, std::placeholders::_1);
	_mCommands[quit.strString] = quit;

	Command listRooms;
	listRooms.strString = "/rooms";
	listRooms.strDescription = "List the available chat rooms.";
	listRooms.Execute = std::bind(&ClientHandler::ListRoomsHandler, this, std::placeholders::_1);
	_mCommands[listRooms.strString] = listRooms;

	Command joinRoom;
	joinRoom.strString = "/join";
	joinRoom.strDescription = "Join the specified chat room.  Creates the room if it doesn't exist.";
	joinRoom.Execute = std::bind(&ClientHandler::JoinRoomHandler, this, std::placeholders::_1);
	_mCommands[joinRoom.strString] = joinRoom;

	Command leaveRoom;
	leaveRoom.strString = "/leave";
	leaveRoom.strDescription = "Leaves the current chat room.";
	leaveRoom.Execute = std::bind(&ClientHandler::LeaveRoomHandler, this);
	_mCommands[leaveRoom.strString] = leaveRoom;

	Command who;
	who.strString = "/who";
	who.strDescription = "Prints the list of people in the given chat room.  " 
											 "If no room is specified, shows everyone connected.";
	who.Execute = std::bind(&ClientHandler::WhoHandler, this, std::placeholders::_1);
	_mCommands[who.strString] = who;

	Command msg;
	msg.strString = "/msg";
	msg.strDescription = "Send private message to user (/msg wilbur salutations!)";
	msg.Execute = std::bind(&ClientHandler::MsgHandler, this, std::placeholders::_1);
	_mCommands[msg.strString] = msg;

	Command help;
	help.strString = "/help";
	help.strDescription = "Prints this list, but only when you recognize you need to ask for it.";
	help.Execute = std::bind(&ClientHandler::ListCommands, this);
	_mCommands[help.strString] = help;
}

ChatServer::ClientHandler::~ClientHandler()
{
	_cm.RemoveClient(this);
	close(_iSocketFD);
}

void ChatServer::ClientHandler::HandleClient()
{
	int result = 0;

	try
	{
		// Make them login first
		LoginHandler();

		while(!_bDone)
		{
			// See if there's a message from the client
			if(!DataPending())
			{
				// If it's been too long since they sent anything, assume they DCd
				auto duration = duration_cast<seconds>(steady_clock::now() - _tLastRead);
				if(duration.count() > MAX_IDLE_SECONDS)
				{
					// Assume they've DCd
					syslog(
						LOG_NOTICE, 
						"Kicking inactive client %s", 
						_strUserName.c_str()
						);
					WriteString("You've been idle for too long.\n");
					break;
				}

				// Nothing to read, or empty message
				// So sleep for a bit - don't want to spin as fast as possible...
				std::this_thread::sleep_for(std::chrono::milliseconds(250));

				// Start over
				continue;
			}

			string msg = ReadString(); 
			CommandMessage pcmd;

			// Check to see if we've got a command or a generic chat message
			if(ParseCommand(msg, std::ref(pcmd)))
			{
				_mCommands[pcmd.CommandString].Execute(pcmd.Args);
			}
			else if(_strCurrentRoom != "") 
			{
				// If they're in a chat room, post a msg
				_cm.PostMsgToRoom(msg, _strCurrentRoom, _strUserName);
			}
			else
			{
				// They're not in a chat room, and they didn't issue a command
				// So suggest something helpful
				WriteString("Please join a chat room before posting a message.\n");
				ListCommands();
			}
		}
	} 
	catch(const std::runtime_error& ex)
	{
		syslog(
			LOG_ALERT, 
			"ClientHandler::HandleClient()> Runtime error: %s", 
			ex.what()); 
	}
	catch(...)
	{
		syslog(LOG_ALERT, "ClientHandler::HandleClient()> GREMLINS DETECTED!"); 
	}

	ShutdownConnection();
}

//---------------------------------------------------------
// Command handler functions
//---------------------------------------------------------
void ChatServer::ClientHandler::ListRoomsHandler(const std::string& args)
{
	std::ostringstream str;
	auto roomNames = _cm.GetRooms();

	// If there aren't any rooms, let them know.
	if(roomNames.size() <= 0)
	{
		WriteString("No active rooms.  Make one with '/join " 
							  + _strUserName + "sPartyTimeLounge'!\n");
		return;
	}

	str << "Active rooms are: " << endl;
	for(auto& room: roomNames)
	{
		auto occupants = _cm.GetUsersIn(room);
		str << "  * " << room << "(" << occupants.size() << ")"; 
		if(_strCurrentRoom == room)
		{
			str << " <-- you are here";
		}
		str << endl;
	}
	str << "end of list" << endl;
	WriteString(str.str());
}

void ChatServer::ClientHandler::JoinRoomHandler(const std::string& args)
{
	// Make sure they're not furiously standing still
	if(_cm.ToUpper(args) == _cm.ToUpper(_strCurrentRoom))
	{
		WriteString("You stay in " + _strCurrentRoom + "...\n");
		return;
	}

	// Make sure the name doesn't have any weird characters
	for(int i = 0; i < args.length(); ++i)
	{
		// If the current letter isn't A-Za-z0-9, try again.
		if(!(('A' <= args[i] && args[i] <= 'Z') ||
		     ('a' <= args[i] && args[i] <= 'z') || 
				 ('0' <= args[i] && args[i] <= '9')))
		{
			WriteString("Invalid room name - only letters and numbers allowed.\n");
			return;
		}
	}

	_cm.SwitchRoom(_strCurrentRoom, args, this);
	WriteString("entering room: " + _strCurrentRoom + "\n");
	WhoHandler(args);
}

void ChatServer::ClientHandler::WhoHandler(const std::string& args)
{
	std::ostringstream str;
	string roomName = _cm.GetProperRoomName(args);

	if(roomName == "" && args != "")
	{
		// The room name they specified is not valid
		str << "Room '" << args << "' does not exist." << endl;
		WriteString(str.str());
		return;
	}

	// Get the list of users in the given room
	auto users = _cm.GetUsersIn(roomName);

	// If they asked for an empty room...
	if(users.size() == 0 && roomName != "")
	{
		str << "Room '" << roomName << "' is empty." << endl;
		WriteString(str.str());
		return;
	}

	// If they asked for ALL users
	if(args == "")
	{
		str << "Users connected: " << endl;
	}
	// If they asked for a valid room
	else
	{
		str << "Users in " << roomName << ":" << endl;
	}
	for(auto& user: users)
	{
		str << "  * " << user; 
		if(user == _strUserName)
		{
			str << " (** this is you)";
		}
		str << endl;
	}
	str << "end of list" << endl;
	WriteString(str.str());
}

void ChatServer::ClientHandler::MsgHandler(const std::string& args)
{
	// Extract the target name from the args (should be first word shape)
	string dest = "";
	int i;
	for(i = 0; i < args.length(); ++i)
	{
		if(('a' <= args[i] && args[i] <= 'z') ||
		   ('A' <= args[i] && args[i] <= 'Z'))
		{
			dest += args[i];
		}
		else
		{
			break;
		}
	}

	if(dest.length() <= 0)
	{
		WriteString("Invalid destination user specified.\n");
		return;
	}

	// This is an indication of madness. 
	if(_cm.ToUpper(dest) == _cm.ToUpper(_strUserName))
	{
		WriteString("Talking to yourself again, eh " + _strUserName + "?\n");
		return;
	}

	// See if that's a valid user name
	if(!_cm.DoesUserExist(dest))
	{
		// Invalid user name
		WriteString("User '" + dest + "' does not exist.\n");
		return;
	}

	// Make sure we've got the right representation of the user name
	dest = _cm.GetProperUserName(dest);

	// We found the right user! Now need to validate message
	if(args.length() <= dest.length()+1)
	{
		WriteString("You must specify a message to send to " + dest + 
				"; example: /msg wilbur salutations!\n");
		return;
	}

	// If we made it here, everything's good to go - send the message.
	string msg = args.substr(i+1);
	_cm.SendMsgToUser(msg, _strUserName, dest);
}

void ChatServer::ClientHandler::LeaveRoomHandler()
{
	// Sanity check
	if(_strCurrentRoom == "")
	{
		WriteString("You can't leave a room you never joined...\n");
		return;
	}
	_cm.SwitchRoom(_strCurrentRoom, "", this);
}

void ChatServer::ClientHandler::LoginHandler()
{
	try
	{
		WriteString("Welcome to this world!!\n");

		_strUserName = "";
		int tries_left = 5;
		do
		{
			WriteString("Login Name?\n");
			_strUserName = ReadString();

			if(_strUserName.length() > MAX_USER_NAME_LENGTH)
			{
				WriteString("That name's too long.  Try again!\n");
				_strUserName = "";
				continue;
			}

			// Filter out any invalid chars
			for(int i = 0; i < _strUserName.length(); ++i)
			{
				char c = _strUserName[i];
				if(!(('A' <= c && c <= 'Z') || 
				     ('a' <= c && c <= 'z')))
				{
				 	// If there's an invalid character, TRY AGAIN
					WriteString("Invalid user name: only letters allowed. Try again!\n");
					_strUserName = "";
					break;
				}
			}
			// If we cleared the user name in the previous loop, try again
			if(_strUserName == "")
			{
				continue;
			}

			// Check for name collision
			if(_cm.DoesUserExist(_strUserName))
			{
				WriteString("That name is taken.  Try again!\n");

				// Try again, name was taken
				_strUserName = "";
				continue;
			} 

			_cm.AddClient(this);

		} while(_strUserName == "" && --tries_left > 0);

		if(tries_left <= 0)
		{
			WriteString("Max number of attempts reached.  "  
									"No soup for you!  Come back one year!\n");
			Bail("too many invalid login attempts");
		}

		WriteString("Welcome, " + _strUserName + "\n");
		ListCommands();
	}
	catch(const std::runtime_error& ex)
	{
		syslog(LOG_NOTICE, "Problem with login: %s",ex.what());
		_bDone = true;
	}
}

void ChatServer::ClientHandler::QuitHandler(std::string args)
{
	_cm.RemoveClient(this);
	WriteString("BYE\n");
	_bDone = true;
}

//---------------------------------------------------------
// Utility functions
//---------------------------------------------------------

void ChatServer::ClientHandler::ShutdownConnection()
{
	// Don't want any more reading / writing
	std::lock_guard<std::mutex> lock(_mMutex);

	try
	{
		// Tell the main loop we're done
		_bDone = true;

		// Tell the OS we're not writing any more
		shutdown(_iSocketFD, SHUT_WR);

		// Put the socket in non-blocking mode
		// (we don't want a blocking read call to hold us up)
		fcntl(_iSocketFD, F_SETFL, O_NONBLOCK);

		// Read any remaining data, throw it away
		const int BUF_SIZE = 256;
		char buf[BUF_SIZE];
		int bytesRead = 0;
		do
		{
			bytesRead = recv(_iSocketFD, buf, BUF_SIZE, 0);
		}
		while(bytesRead > 0);

		// Tell the OS we're not reading any more
		shutdown(_iSocketFD, SHUT_RD);
	}
	catch(...)
	{
		// Silently drop errors here - we're shutting down
	}
}

void ChatServer::ClientHandler::ListCommands()
{
	string msg = "Available commands:\n";
	for(auto& item: _mCommands)
	{
		msg += "\t" + item.second.strString + ": " 
		       + item.second.strDescription + "\n";
	}
	WriteString(msg);
}

void ChatServer::ClientHandler::SendMsg(const std::string& msg)
{
	WriteString(msg);
}

std::string ChatServer::ClientHandler::ReadString()
{
	const int BUF_SIZE = 512;
	const int MAX_MSG_SIZE = 1024;
	int bytesRead = 0;
	char buffer[BUF_SIZE];
	memset(buffer, '\0', BUF_SIZE);
	string msg = "";

	// There's something to read - so get a lock and read!
	std::lock_guard<std::mutex> lock(_mMutex);

	// Read all available bytes, create a string, but keep it reasonable
	int bytesAvailable = 0;
	do
	{
		bytesRead = recv(_iSocketFD, buffer, BUF_SIZE, 0);

		if(bytesRead == 0)
		{
			throw std::runtime_error("Client disconnect detected in ReadString()");
		}

		if(bytesRead < 0)
		{
			throw std::runtime_error("could not read from client socket.");
		}

		// Make a note of when this read happened
		_tLastRead = std::chrono::steady_clock::now();

		msg += buffer;
		ioctl(_iSocketFD, FIONREAD, &bytesAvailable);
	}
	while(msg.length() < MAX_MSG_SIZE && bytesAvailable > 0);

	// If we get here, and the buffer is at the cap, but there's more data...
	if(msg.length() >= MAX_MSG_SIZE && bytesAvailable > 0)
	{
		// The client sent way too much stuff, time to kick them
		throw std::runtime_error("client sent too much data");
	}

	return Scrub(msg);
}

void ChatServer::ClientHandler::WriteString(const std::string& msg)
{
	std::lock_guard<std::mutex> lock(_mMutex);

	// Make sure the socket is in a valid state before trying to send
	int error = 0;
	socklen_t len = sizeof (error);
	int retval = getsockopt (_iSocketFD, SOL_SOCKET, SO_ERROR, &error, &len );
	if(retval != 0)
	{
		// Problem with the socket, we should bail
		Bail("socket error detected before writing, aborting");
		return;
	}

	// Send the message
#ifdef __linux
	int flags = 0 | MSG_NOSIGNAL; // <-- Prevents SIGPIPE (among other things)
#else
	int flags = 0;
#endif
	int result = send(_iSocketFD, msg.c_str(), sizeof(char)*msg.length(), flags);
	if(result < 0)
	{
		Bail("could not write to client socket");
	}
}

void ChatServer::ClientHandler::Bail(const std::string err)
{
	syslog(
		LOG_ALERT, 
		"Bailing on client thread for user %s because %s",
		_strUserName.c_str(),
		err.c_str()
		);
	_bDone = true;
}

std::string ChatServer::ClientHandler::Scrub(const std::string& msg)
{
	// Make sure every character is ascii, and it ends with null terminator
	// int of 32-126 inclusive, avoids \t, \n, etc.
	string scrubbed = "";
	for(int i = 0; i < msg.length(); ++i)
	{
		if(msg[i] == '\0' || msg[i] == '\n' || msg[i] == '\r')
		{
			// Found a null-terminator - this should be the end of the string
			// Also, assume that \n or \r signal the end of a message
			return scrubbed;
		}

		if((msg[i] >= 32 && msg[i] <= 126)) 
		{
			scrubbed += msg[i];
		}
    else
    {
      scrubbed += ' ';
    }
	}
	return scrubbed;
}

bool ChatServer::ClientHandler::ParseCommand(
       const std::string& msg, 
			 ChatServer::CommandMessage& pcmd)
{
	// If the string doesn't start with '/', it's not a command.
	if(msg[0] != '/')
	{
		return false;
	}

	string cmd = "/";
	for(int i = 1; i < msg.length(); ++i)
	{
		// Accumulate all the characters until the first non-[a-z] char
		// (after the initial '/')
		if(msg[i] >= 'a' && msg[i] <= 'z')
		{
			cmd += msg[i];
		}
		else
		{
			break;
		}
	}

	// If the length of accumulated chars is 0, we don't have a command.
	if(cmd.length() <= 1)
	{
		return false;
	}

	// We MIGHT have an actual command - search command structure
	if(_mCommands.find(cmd) != _mCommands.end())
	{
		// Found a valid command!
		pcmd.CommandString = cmd;

		// Now might have to deal with args
		if(msg.length() - cmd.length() > 0)
		{
			pcmd.Args = msg.substr(pcmd.CommandString.length()+1);
		}
		else
		{
			pcmd.Args = msg.substr(pcmd.CommandString.length()); 
		}
		return true;
	}
	
	// If we made it here, we didn't find it.
	return false;
}

bool ChatServer::ClientHandler::DataPending()
{
	int bytesAvailable = 0;
	if(ioctl(_iSocketFD, FIONREAD, &bytesAvailable) < 0)
	{
		throw std::runtime_error("Could not run ioctl to get bytes available.");
	}

	return bytesAvailable > 0;
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

void ChatServer::ClientHandler::SetCurrentRoom(const std::string& room)
{
	std::lock_guard<std::mutex> lock(_mMutex);
	_strCurrentRoom = room;
}

bool ChatServer::ClientHandler::StillValid()
{
	return !_bDone;
}
