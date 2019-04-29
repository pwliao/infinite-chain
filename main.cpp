#include <cstdio>
#include <cstdlib>
#include <thread>
#include <iostream>
#include <fstream>
#include "rpc.hpp"
#include "blockchain.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

json readConfig()
{
	string line;
	ifstream file("config.json");
	string json_str = "";
	if (file.is_open()) {
		while (getline(file, line)) {
			json_str += line;
		}
		file.close();
	}
	return json::parse(json_str);
}

int main()
{
	json config = readConfig();
	Blockchain blockchain(config["target"]);
	RpcServer rpcserver;
	thread rpc(&RpcServer::run, &rpcserver, ref(blockchain));
	printf("rpc start\n");
	blockchain.mining();
	rpc.join();
	return 0;
}
