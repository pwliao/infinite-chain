#ifndef RPC_HPP
#define RPC_HPP

#include <string>
#include "blockchain.hpp"

class APIServer
{
public:
	APIServer() = default;
	void run(Blockchain &blockchain, int port);
	virtual std::string getResponse(std::string message, Blockchain &blockchain) = 0;
private:
};

class UserApi: public APIServer
{
public:
	std::string getResponse(std::string message, Blockchain &blockchain) override;
};

class P2PApi: public APIServer
{
public:
	std::string getResponse(std::string message, Blockchain &blockchain) override;
};

#endif
