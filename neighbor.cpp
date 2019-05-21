#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include "neighbor.hpp"
using namespace std;

void Neighbors::broadcast(string message)
{
	cout << "廣播 " << message << endl;
	for (const Neighbor &neighbor : neighbors) {
		int fd = socket(PF_INET, SOCK_STREAM, 0);

		if (fd == -1) {
			printf("socket() error\n");
			continue;
		}
		struct sockaddr_in info;
		memset(&info, 0, sizeof(info));
		info.sin_family = PF_INET;

		info.sin_addr.s_addr = inet_addr(neighbor.ip.c_str());
		info.sin_port = htons(neighbor.port);
		if (connect(fd, (struct sockaddr *)&info, sizeof(info)) == -1) {
			printf("\033[0;32;31m連線至 %s:%d 錯誤\033[m\n", neighbor.ip.c_str(), neighbor.port);
		} else {
			send(fd, message.c_str(), message.length(), 0);
		}
		close(fd);
	}
}

void Neighbors::addNeighbor(Neighbor neighbor)
{
	neighbors.push_back(neighbor);
}
