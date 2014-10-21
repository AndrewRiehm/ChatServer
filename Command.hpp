
#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <functional>

namespace ChatServer
{

struct Command
{
	std::string strString;
	std::string strDescription;
	std::function<void(std::string)> Execute;
};

}
#endif
