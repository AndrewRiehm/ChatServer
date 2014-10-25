#ifndef CHAT_MANAGER_HPP
#define CHAT_MANAGER_HPP

#include <map>
#include <vector>
#include <string>

namespace ChatServer
{

class ClientHandler;

class ChatManager
{
private:
	std::map<std::string, std::vector<std::string> > _mRooms; // room name -> list of user names
	std::map<std::string, ChatServer::ClientHandler*> _mClients; // user name -> client object

	// Removes a user from the room, if they exist, and deletes the room if empty.
	void RemoveUserFromRoom(const std::string& room, const std::string& userName);

public:
	ChatManager();
	~ChatManager();

	/** Gets the current list of room names **/
	std::vector<std::string> GetRooms();

	/** Gets the list of user names in the given room **/
	std::vector<std::string> GetUsersIn(std::string roomName);

	/** Adds the given client to the ChatManager's list. **/
	bool AddClient(ChatServer::ClientHandler* client);

	/** Removes the given client from the internal list. **/
	void RemoveClient(ChatServer::ClientHandler* client);

	/** 
	Switches a given client from one room to another, 
	also used to create and join a room (if 'fromRoom' is
	equal to "").
	**/
	void SwitchRoom(
	       const std::string fromRoom, 
				 const std::string toRoom, 
				 ChatServer::ClientHandler* client
				 );

	/** Posts the given message to all users in the given room **/
	void PostMsgToRoom(
	       const std::string& msg, 
				 const std::string& roomName, 
				 const std::string& fromUser);

	/** Sends a private message from the specified user to the other one. **/
	void SendMsgToUser(
	     	const std::string& msg, 
				const std::string& fromUser, 
				const std::string& toUser
				);
};

}

#endif
