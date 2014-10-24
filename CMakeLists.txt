cmake_minimum_required (VERSION 2.8.1)
project (ChatServer)

add_definitions(-std=c++11)

add_executable(chat_server Server.cpp ClientHandler.cpp ChatManager.cpp)
