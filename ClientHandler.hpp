#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include <map>
#include <mutex>
#include <string>
#include <chrono>
#include "Command.hpp"

namespace ChatServer
{

// Forward declaration to avoid circular #include references.
class ChatManager;

/**
	Used as return value from ClientHandler::ParseCommand, to encapsulate both a 
	command string and the remaining arguments.
**/
struct CommandMessage
{
	std::string CommandString;
	std::string Args;
};

/**
	Handles all interaction with a given client.

	Takes a file descriptor in the constructor, and is intended to manage
	all sending to and receiving from the client connected on that descriptor.

	Designed to run ClientHandler::HandleClient() in its own thread.  Note that
	the methods ReadString() and WriteString() are blocking (synchronous).
**/
class ClientHandler
{
private:
	const int MAX_USER_NAME_LENGTH = 30; // Make sure user names aren't too big
	const int MAX_IDLE_SECONDS = 300; // If no msgs in 5 minutes, kick them!

	std::mutex _mMutex; // To avoid threading issues when reading/writing
	ChatManager& _cm;
	int _iSocketFD; // Socket for talking to the client
	std::map<std::string, ChatServer::Command> _mCommands; // Command structure
	std::string _strUserName; // User name associated with this connection
	std::string _strCurrentRoom; // Name of room this user is currently in
	bool _bDone; // Set to false to kill the connection and stop the thread
	std::chrono::steady_clock::time_point _tLastRead; // Detecting DCs/inactive

	/** Shuts down the socket and cleans up any remaining data **/
	void ShutdownConnection();

	/**  Sends a list of commands to the connected client **/
	void ListCommands();

	/** 
	Reads a string from the client, scrubs it and returns a std::string
	WARNING - THIS WILL BLOCK THE CLIENT - you can't read or write from 
	other threads while this is waiting for input. 
	**/
	std::string ReadString(); 

	/** Writes a string to the client **/
	void WriteString(const std::string& msg);

	/** Scrubs the buffer for invalid characters, returns a string version **/
	std::string Scrub(const std::string& msg);

	/** Checks to see if data from the client is waiting on the socket **/
	bool DataPending();

	/** Checks to see if the given message is a valid command **/
	bool IsCommand(const std::string& msg);

	/** Parses the given message into a CommandMessage object **/
	bool ParseCommand(const std::string& msg, ChatServer::CommandMessage& pcmd);

	/** In case of emergency, call this **/
	void Bail(const std::string err);

	/** Handles user authentication **/
	void LoginHandler();

	/** Handles the /quit command **/
	void QuitHandler(std::string args);

	/** Handles joining a new room **/
	void JoinRoomHandler(const std::string& args);

	/** Handles leaving the current room **/
	void LeaveRoomHandler();

	/** Lists the given rooms **/
	void ListRoomsHandler(const std::string& args);

	/** Lists the people in the room **/
	void WhoHandler(const std::string& args);

	/** Sends a private message **/
	void MsgHandler(const std::string& args);

public:
	ClientHandler(int fd, ChatManager& cm);
	~ClientHandler();

	/** Main loop **/
	void HandleClient();

	/** Returns the user's name **/
	std::string GetUserName() const;

	/** Returns the current room name **/
	std::string GetCurrentRoom() const;

	/** Sets the current room value **/
	void SetCurrentRoom(const std::string& room);

	/** Sends the given message to the user. **/
	void SendMsg(const std::string& msg);

	/** If this returns false, this client is going away soon. **/
	bool StillValid();
};

}
#endif
