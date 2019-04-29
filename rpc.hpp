#ifndef RPC_HPP
#define RPC_HPP

#include <string>
#include "blockchain.hpp"

class RpcServer
{
public:
	RpcServer();
	void run(Blockchain &blockchain);
	std::string getResponse(std::string message, Blockchain &blockchain);
private:
};

#endif
