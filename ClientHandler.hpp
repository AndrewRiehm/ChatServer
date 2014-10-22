#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include <map>
#include <string>
#include "Command.hpp"

namespace ChatServer
{

class ClientHandler
{
private:
	int _iSocketFD;
	std::map<std::string, ChatServer::Command> _mCommands;

	// Sends a list of commands to the connected client
	void ListCommands();

	// Reads a string from the client, scrubs it and returns a std::string
	std::string ReadString(); 

	// Writes a string to the client
	void WriteString(std::string msg);

	// Scrubs the given buffer for invalid characters, returns a std::string version
	std::string Scrub(const char* buf, const int buf_size);

	// In case of emergency, call this
	void Bail(std::string err);

public:
	ClientHandler(int fd);
	~ClientHandler();

	void HandleClient();
	void LoginHandler(std::string args);
};

}
#endif
