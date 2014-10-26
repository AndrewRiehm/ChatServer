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
}

ChatManager::~ChatManager()
{
}

string ChatManager::GetProperUserName(const std::string& user)
{
	string capsUser = ToUpper(user);
	for(auto client: _mClients)
	{
		if(ToUpper(client.first) == capsUser)
		{
			return client.first;
		}
	}
	return "";
}

string ChatManager::GetProperRoomName(const std::string& room)
{
	string capsRoom = ToUpper(room);
	for(auto room: _mRooms)
	{
		if(ToUpper(room.first) == capsRoom)
		{
			return room.first;
		}
	}
	return "";
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
	vector<string> ret;
	if(roomName == "")
	{
		// Get ALL the users!
		for(auto& client: _mClients)
		{
			ret.push_back(client.first);
		}
		return ret;
	}

	if(_mRooms.find(roomName) == _mRooms.end())
	{
		// Room doesn't exist, so nobody in it!
		return ret;
	}

	auto& room = _mRooms[roomName];
	for(auto& user: room)
	{
		ret.push_back(user);
	}
	return ret;
}

bool ChatManager::AddClient(ChatServer::ClientHandler* client)
{
	// Only add the client if the user name does not already exist.
	string userName = client->GetUserName();

	std::lock_guard<std::mutex> lock(_mMutex);
	if(_mClients.find(userName) == _mClients.end())
	{
		_mClients[userName] = client;
		return true;
	}
	return false;
}

void ChatManager::RemoveUserFromRoom(const std::string& room, const std::string& userName)
{
	std::lock_guard<std::mutex> lock(_mMutex);

	// Remove the user from the room they're in,
	// if they're in a room
	string capsUser = ToUpper(userName);
	if(room != "" && _mRooms.find(room) != _mRooms.end())
	{
		auto names = _mRooms[room];
		for(int i = 0; i < names.size(); ++i)
		{
			if(ToUpper(names[i]) == capsUser)
			{	
				// We found the user in the room, now need to delete them and notify
				names.erase(names.begin()+i);
				if(names.size() > 0)
				{
					// If there are still people in the room, save that
					_mRooms[room] = names;
					PostMsgToRoom(userName + " has left " + room, room, "");
				}
				else
				{
					// If there's nobody in the room, delete it.
					_mRooms.erase(room);
				}

				// If the user is still connected, send a notification
				if(_mClients.find(userName) != _mClients.end())
					_mClients[userName]->SendMsg("* You have left " + room + "\n");
				break;
			}
		}
	}
}

std::string ChatServer::ChatManager::ToUpper(const std::string& msg)
{
	string s = "";
	for(int i = 0; i < msg.length(); ++i)
	{
		s += toupper(msg[i]);
	}
	return s;
}

bool ChatManager::DoesUserExist(const std::string& user)
{
	string capsUser = ToUpper(user);
	for(auto& client: _mClients)
	{
		string name = ToUpper(client.second->GetUserName());
		if(name == capsUser)
		{
			return true;
		}
	}
	return false;
}

void ChatManager::RemoveClient(ChatServer::ClientHandler* client)
{
	string room = client->GetCurrentRoom();
	string userName = client->GetUserName();

	// Remove the user from the list of clients
	{
		std::lock_guard<std::mutex> lock(_mMutex);
		auto it=_mClients.find(userName);
		if(it != _mClients.end())
		{
			_mClients.erase(it);
		}
	}

	RemoveUserFromRoom(room, userName);
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

	string dest = "";

	// Find the toRoom, if it exists
	if(toRoom != "")
	{
		string capsTo = ToUpper(toRoom);
		for(auto r: _mRooms)
		{
			if(ToUpper(r.first) == capsTo)
			{
				// Found the matching room
				dest = r.first;
			}
		}
		if(dest == "")
		{
			dest = toRoom;
			// Need to create it, doesn't exist yet.
			_mRooms[dest] = vector<string>();
		}
		// Add the client's name to the room
		_mRooms[dest].push_back(client->GetUserName());

		// Tell everyone about the new member
		PostMsgToRoom("new user joined chat: " + client->GetUserName(), dest, "");
	}

	client->SetCurrentRoom(dest);
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
	string  m = "";
	if(fromUser != "")
	{
		m = "[" + roomName + "] " + fromUser + ": " + msg + "\n";
	}
	else
	{
		m = "* " + msg + "\n";
	}

	// Send it to all associated users
	for(auto& user: users)
	{
		_mClients[user]->SendMsg(m);
	}
}

void ChatManager::SendMsgToUser(const string& msg, const string& fromUser, const string& toUser)
{
	string capsToUser = ToUpper(toUser);
	string capsFromUser = ToUpper(fromUser);
	ClientHandler* clientTo = NULL; 
	ClientHandler* clientFrom = NULL;

	// Find the clients (to and from) in a case-insensitive way
	for(auto client: _mClients)
	{
		string name = ToUpper(client.second->GetUserName());
		if(name == capsToUser)
		{
			clientTo = client.second;
		}
		else if(name == capsFromUser)
		{
			clientFrom = client.second;
		}
	}

	if(clientTo == NULL)
	{
		// If we don't have a valid destination, then this makes no sense
		// COMPLAIN LOUDLY!
		throw std::runtime_error("Invalid users in SendMsgToUser (" + fromUser + ", " + toUser + ")");
	}

	if(clientFrom != NULL)
	{
		// We might not have a valid "from" user - but if we do, show this
		clientFrom->SendMsg("You whisper to " + clientTo->GetUserName() + ": " + msg + "\n");

		// Send the message to the target
		clientTo->SendMsg(clientFrom->GetUserName() + " whispers: " + msg + "\n");
	}
	else
	{
		// Send the message to the target
		clientTo->SendMsg(fromUser + " whispers: " + msg + "\n");

		// The "from" might be from the sys admin, or $DEITY, or an AI, in which
		// case we just show "$DEITY whispers: <msg>", but we don't need to (and 
		// probably can't) show $DEITY the corresponding message, because they're
		// not in the list of clients.
	}
}

