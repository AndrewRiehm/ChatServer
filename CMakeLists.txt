cmake_minimum_required (VERSION 2.8.1)
project (ChatServer)

add_definitions(-std=c++11)

add_executable(chatd main.cpp ClientHandler.cpp ChatManager.cpp)

target_link_libraries(chatd pthread)
