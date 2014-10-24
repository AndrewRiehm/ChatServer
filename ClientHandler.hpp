#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include <map>
#include <string>
#include "Command.hpp"

namespace ChatServer
{

class ChatManager;

class ClientHandler
{
private:
	ChatManager& _cm;
	int _iSocketFD; // Socket for talking to the client
	std::map<std::string, ChatServer::Command> _mCommands; // Command structure
	std::string _strUserName;
	std::string _strCurrentRoom;

	// Sends a list of commands to the connected client
	void ListCommands();

	// Reads a string from the client, scrubs it and returns a std::string
	std::string ReadString(); 

	// Writes a string to the client
	void WriteString(const std::string& msg);

	// Scrubs the given buffer for invalid characters, returns a std::string version
	std::string Scrub(const char* buf, const int buf_size);

	// In case of emergency, call this
	void Bail(std::string err);

	// Handles user authentication
	void LoginHandler(std::string args);

	// Handles the /quit command
	void QuitHandler(std::string args);


public:
	ClientHandler(int fd, ChatManager& cm);
	~ClientHandler();

	// Main loop
	void HandleClient();

	// Returns the user's name
	std::string GetUserName() const;

	// Returns the current room name
	std::string GetCurrentRoom() const;

	/** Sends the given message to the user. **/
	void SendMsg(const std::string& msg);
};

}
#endif
