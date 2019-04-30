#ifndef RPC_HPP
#define RPC_HPP

#include <string>
#include "blockchain.hpp"

class RpcServer
{
public:
	RpcServer() = default;
	void run(Blockchain &blockchain, int port);
	virtual std::string getResponse(std::string message, Blockchain &blockchain) = 0;
private:
};

class UserApi: public RpcServer
{
public:
	std::string getResponse(std::string message, Blockchain &blockchain) override;
};

class P2PApi: public RpcServer
{
public:
	std::string getResponse(std::string message, Blockchain &blockchain) override;
};

#endif
