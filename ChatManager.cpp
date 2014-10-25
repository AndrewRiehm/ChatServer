#include <iostream>

#include "ChatManager.hpp"
#include "ClientHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;

using std::vector;
using std::map;
using std::string;

using ChatServer::ChatManager;


ChatManager::ChatManager()
{
	cout << "ChatManager constructed!" << endl;
}

ChatManager::~ChatManager()
{
	cout << "ChatManager deconstructed!" << endl;
}

vector<string> ChatManager::GetRooms()
{
	// TODO: Could this be more efficient, by storing references instead of 
	//       creating new strings?
	vector<string> ret;
	for(auto& room: _mRooms)
	{
		ret.push_back(room.first);
	}
	return ret;
}


vector<string> ChatManager::GetUsersIn(string roomName)
{
	// TODO: MAKE SURE roomName exists!

	vector<string> ret;
	auto& room = _mRooms[roomName];
	for(auto& user: room)
	{
		ret.push_back(user);
	}
	return ret;
}

bool ChatManager::AddClient(ChatServer::ClientHandler* client)
{
	// TODO: THREAD SAFETY
	cout << __func__ << ": " << client->GetUserName() << endl;

	// Only add the client if the user name does not already exist.
	string userName = client->GetUserName();
	if(_mClients.find(userName) == _mClients.end())
	{
		_mClients[userName] = client;
		return true;
	}
	return false;
}

void ChatManager::RemoveUserFromRoom(const std::string& room, const std::string& userName)
{
	// TODO: THREAD SAFETY!

	// Remove the user from the room they're in,
	// if they're in a room
	if(room != "" && _mRooms.find(room) != _mRooms.end())
	{
		auto names = _mRooms[room];
		for(int i = 0; i < names.size(); ++i)
		{
			if(names[i] == userName)
			{	
				names.erase(names.begin()+i);
				if(names.size() > 0)
				{
					// If there are still people in the room, save that
					_mRooms[room] = names;
				}
				else
				{
					// If there's nobody in the room, delete it.
					_mRooms.erase(room);
				}
				break;
			}
		}
	}
}

void ChatManager::RemoveClient(ChatServer::ClientHandler* client)
{
	// TODO: Thread safety!
	string room = client->GetCurrentRoom();
	string userName = client->GetUserName();

	RemoveUserFromRoom(room, userName);

	// Remove the user from the list of clients
	auto it=_mClients.find(client->GetUserName());
	if(it != _mClients.end())
	{
		_mClients.erase(it);
	}
}

void ChatManager::SwitchRoom(
	       const string fromRoom, 
				 const string toRoom, 
				 ClientHandler* client)
{
	// First remove the user from the old room, if one is specified
	if(fromRoom != "")
	{
		RemoveUserFromRoom(fromRoom, client->GetUserName());
	}

	// Find the toRoom, if it exists
	if(toRoom != "")
	{
		auto newRoom = _mRooms.find(toRoom);
		if(newRoom == _mRooms.end())
		{
			// Need to create it, doesn't exist yet.
			_mRooms[toRoom] = vector<string>();
		}
		// Add the client's name to the room
		_mRooms[toRoom].push_back(client->GetUserName());
	}

	client->SetCurrentRoom(toRoom);
}

void ChatManager::PostMsgToRoom(
			const std::string& msg, 
			const std::string& roomName, 
			const std::string& fromUser)
{
	// Make sure room exists!
	if(_mRooms.find(roomName) == _mRooms.end())
	{
		_mClients[fromUser]->SendMsg("Invalid room (" + roomName + ")!\n");
		return;
	}

	auto users = _mRooms[roomName];

	// Format the message
	string m = "[" + roomName + "] " + fromUser + ": " + msg + "\n";

	// Send it to all associated users
	for(auto& user: users)
	{
		_mClients[user]->SendMsg(m);
	}
}

void ChatManager::SendMsgToUser(const string& msg, const string& fromUser, const string& toUser)
{
	_mClients[toUser]->SendMsg(fromUser + " whispers: " + msg);
}
