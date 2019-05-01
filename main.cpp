#include <cstdio>
#include <cstdlib>
#include <thread>
#include <iostream>
#include <fstream>
#include "api.hpp"
#include "blockchain.hpp"
#include "json.hpp"
#include "transaction.hpp"

using json = nlohmann::json;
using namespace std;

json readConfig()
{
	string line;
	ifstream file("config.json");
	string json_str;
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
	Blockchain blockchain(config);

	UserApi userapi;
	P2PApi p2papi;
	thread user(&APIServer::run, &userapi, ref(blockchain), config["user_port"]);
	thread p2p(&APIServer::run, &p2papi, ref(blockchain), config["p2p_port"]);
	printf("api server start\n");

	if (config["mining"] == true) {
        printf("mining start\n");
        blockchain.mining();
	}

	user.join();
	p2p.join();
	return 0;
}
