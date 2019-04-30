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

string block_str = "{\n"
               "    \"method\": \"sendBlock\",\n"
               "    \"height\": 1,\n"
               "    \"data\": {\n"
               "        \"version\": 2,\n"
               "        \"prev_block\": \"0000be5b53f2dc1a836d75e7a868bf9ee576d57891855b521eaabfa876f8a606\",\n"
               "        \"transactions_hash\": \"\",\n"
               "        \"beneficiary\": \"3b04799fe6ed537b4e8e1df872df6223284779f0\",\n"
               "        \"target\": \"0001000000000000000000000000000000000000000000000000000000000000\",\n"
               "        \"nonce\": 1,\n"
               "        \"transactions\": [\n"
               "            {\n"
               "                \"nonce\": 1,\n"
               "                \"sender_pub_key\": \"0x4643bb6b393ac20a6175c713175734a72517c63d6f73a3ca90a15356f2e967da03d16431441c61ac69aeabb7937d333829d9da50431ff6af38536aa262497b27\",\n"
               "                \"to\": \"3b04799fe6ed537b4e8e1df872df6223284779f0\",\n"
               "                \"value\": 10,\n"
               "                \"fee\": 1,\n"
               "                \"signature\": \"\"            \n"
               "            }\n"
               "        ]\n"
               "    }\n"
               "}";

int main()
{
    Block block(block_str);
	json config = readConfig();
	Blockchain blockchain(config["target"]);
	UserApi userapi;
	P2PApi p2papi;
	thread user(&APIServer::run, &userapi, ref(blockchain), config["user_port"]);
	thread p2p(&APIServer::run, &p2papi, ref(blockchain), config["p2p_port"]);
	printf("api server start\n");
	blockchain.mining();
	user.join();
	p2p.join();
	return 0;
}
