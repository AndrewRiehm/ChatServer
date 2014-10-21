#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include <map>
#include "Command.hpp"

namespace ChatServer
{

class ClientHandler
{
private:
	int _iSocketFD;
	std::map<std::string, ChatServer::Command> _mCommands;

	void ListCommands();

public:
	ClientHandler(int fd);
	~ClientHandler();

	void HandleClient();
};

void LoginHandler(int fd, std::string args);
}
#endif
