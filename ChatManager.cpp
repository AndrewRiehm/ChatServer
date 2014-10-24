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

void ChatManager::RemoveClient(ChatServer::ClientHandler* client)
{
	// TODO: Thread safety!
	cout << __func__ << ": " << client->GetUserName() << endl;

	// TODO: Remove the client from the room they're connected to!
	auto it=_mClients.find(client->GetUserName());
	if(it != _mClients.end())
	{
		_mClients.erase(it);
	}
}

void ChatManager::SwitchRoom(
	       const string fromRoom, 
				 const string toRoom, 
				 ClientHandler& client)
{
	// First remove the user from the old room, if one is specified
	if(fromRoom != "")
	{
		auto oldRoom = _mRooms.find(fromRoom);
		if(oldRoom != _mRooms.end())
		{
			int index = 0;
			for(auto& name: oldRoom->second)
			{
				if(client.GetUserName() == name)
				{
					break;
				}
				++index;
			}
			if(index < oldRoom->second.size())
			{
				oldRoom->second.erase(oldRoom->second.begin()+index);
			}
		}
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
		_mRooms[toRoom].push_back(client.GetUserName());
	}
}

void ChatManager::PostMsgToRoom(const std::string& msg, const std::string& roomName)
{
	// TODO: Make sure room exists!
	auto users = _mRooms[roomName];
	for(auto& user: users)
	{
		_mClients[user]->SendMsg(msg);
	}
}

void ChatManager::SendMsgToUser(const string& msg, const string& fromUser, const string& toUser)
{
	_mClients[toUser]->SendMsg(fromUser + " whispers: " + msg);
}
