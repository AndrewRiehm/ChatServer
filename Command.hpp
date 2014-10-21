
#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>

namespace ChatServer
{

struct Command
{
	std::string strString;
	std::string strDescription;
	void (*Execute)(int, std::string);
};

}
#endif
