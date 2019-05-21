#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <utility>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "blockchain.hpp"
#include "api.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

void APIServer::run(Blockchain &blockchain, int port)
{
	char input_buffer[4096] = {};
	map<int, string> buffer;
	map<int, pair<int,int>> brackets_count;
	int server_fd = 0, client_fd = 0;
	server_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("socket() error\n");
		exit(1);
	}
	struct sockaddr_in server_addr, client_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		printf("bind error\n");
		exit(1);
	}
	if (listen(server_fd, 5) == -1) {
		printf("listen error\n");
		exit(1);
	}
	fd_set reads, copy_reads;
	FD_ZERO(&reads);
	FD_SET(server_fd, &reads);
	int fd_max = server_fd;
	while (1) {
		copy_reads = reads;
		int fd_num = select(fd_max + 1, &copy_reads, 0, 0, 0);
		if (fd_num == -1) {
			fprintf(stderr, "select error\n");
			break;
		} else if (fd_num == 0) {
			continue;
		}
		for (int i = 0; i < fd_max + 1; i++) {
			if (FD_ISSET(i, &copy_reads)) {
				if (i == server_fd) { // connection
					socklen_t addr_size = sizeof(client_addr);
					client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_size);
					if (client_fd == -1) {
						fprintf(stderr, "accept error\n");
					}
					FD_SET(client_fd, &reads);
					if (fd_max < client_fd) {
						fd_max = client_fd;
					}
					fprintf(stderr, "新連線\n");
				} else { // read message
					memset(input_buffer, 0, sizeof(input_buffer));
					int len = read(i, input_buffer, sizeof(input_buffer));
					if (len == 0) {
						FD_CLR(i, &reads);
						close(i);
						fprintf(stderr, "關閉連線\n");
						buffer.erase(i);
					} else if (len == -1) {
						fprintf(stderr, "read error: %s\n", strerror(errno));
					} else {
						fprintf(stderr, "收到請求 %s\n", input_buffer);
						int start_pos = 0;
						for (int j = 0; j < len; j++) {
							if (input_buffer[j] == '{') {
								brackets_count[i].first++;
							} else if (input_buffer[j] == '}') {
								brackets_count[i].second++;
							}
							if (brackets_count[i].first == brackets_count[i].second
									&& brackets_count[i].first != 0) {
								buffer[i] += string(input_buffer).substr(start_pos, j - start_pos + 1);
								start_pos = j + 1;
								brackets_count[i] = make_pair(0, 0);
								string ret = getResponse(buffer[i], blockchain);
								ret += "\n";
								write(i, ret.c_str(), ret.length());
								buffer[i] = "";
							}
						}
						buffer[i] += string(input_buffer).substr(start_pos, len - start_pos + 1);
					}
				}
			}
		}
	}
}

string UserApi::getResponse(string message, Blockchain &blockchain)
{
	try {
		json j = json::parse(message);
		string method = j["method"];
		if (method == "getBlockCount") {
			json response;
			response["result"] = blockchain.getBlockCount();
			return response.dump();
		} else if (method == "getBlockHash") {
		} else if (method == "getBlockHeader") {
		} else if (method == "getbalance") {
			json response;
			response["error"] = 0;
			response["balance"] = blockchain.getBalance(j["data"]["address"]);
			return response.dump();
		} else if (method == "sendtoaddress") {
			blockchain.sendToAddress(j["data"]["address"], j["data"]["amount"]);
			json response;
			response["error"] = 0;
			return response.dump();
		}
	} catch (exception &e) {
		fprintf(stderr, "exception: %s\n", e.what());
		fprintf(stderr, "getResponse error\n");
		json response;
		response["error"] = 1;
		return response.dump();
	}
	return "";
}

string P2PApi::getResponse(string message, Blockchain &blockchain)
{
	try {
		json j = json::parse(message);
		string method = j["method"];
		if (method == "sendBlock") {
			json response;
			if (blockchain.addBlock(Block(message))) {
				response["error"] = 0;
			} else {
				response["error"] = 1;
			}
			return response.dump();
		} else if (method == "getBlocks") {
		} else if (method == "sendTransaction") {
			json response;
			string tx_str = j["data"].dump();
			if (blockchain.addRemoteTransaction(tx_str)) {
				response["error"] = 0;
			} else {
				response["error"] = 1;
			}
			return response.dump();
		}
	} catch (exception &e) {
		fprintf(stderr, "exception: %s\n", e.what());
		fprintf(stderr, "getResponse error\n");
		json response;
		response["error"] = 1;
		return response.dump();
	}
	return "";
}
