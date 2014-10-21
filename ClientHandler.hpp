#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP
class ClientHandler
{
private:
	int _iSocketFD;

public:
	ClientHandler(int fd);
	~ClientHandler();

	void HandleClient();
};
#endif
