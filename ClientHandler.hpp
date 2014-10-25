#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include <map>
#include <string>
#include "Command.hpp"

namespace ChatServer
{

class ChatManager;

struct CommandMessage
{
	std::string CommandString;
	std::string Args;
};

class ClientHandler
{
private:
	ChatManager& _cm;
	int _iSocketFD; // Socket for talking to the client
	std::map<std::string, ChatServer::Command> _mCommands; // Command structure
	std::string _strUserName; // User name associated with this connection
	std::string _strCurrentRoom; // Name of room this user is currently in
	bool _bDone; // Set to false to kill the connection and stop the thread

	// Sends a list of commands to the connected client
	void ListCommands();

	// Reads a string from the client, scrubs it and returns a std::string
	std::string ReadString(); 

	// Writes a string to the client
	void WriteString(const std::string& msg);

	// Scrubs the given buffer for invalid characters, returns a std::string version
	std::string Scrub(const std::string& msg);

	// Checks to see if the given message is a valid command
	bool IsCommand(const std::string& msg);

	// Parses the given message into a CommandMessage object
	bool ParseCommand(const std::string& msg, ChatServer::CommandMessage& pcmd);

	// In case of emergency, call this
	void Bail(const std::string err);

	// Handles user authentication
	void LoginHandler();

	// Handles the /quit command
	void QuitHandler(std::string args);

	// Handles joining a new room
	void JoinRoomHandler(const std::string& args);

	// Lists the given rooms
	void ListRoomsHandler(const std::string& args);

	// Lists the people in the room
	void ListPeepsHandler(const std::string& args);

public:
	ClientHandler(int fd, ChatManager& cm);
	~ClientHandler();

	// Main loop
	void HandleClient();

	// Returns the user's name
	std::string GetUserName() const;

	// Returns the current room name
	std::string GetCurrentRoom() const;

	// Sets the current room value
	void SetCurrentRoom(const std::string& room);

	/** Sends the given message to the user. **/
	void SendMsg(const std::string& msg);
};

}
#endif
