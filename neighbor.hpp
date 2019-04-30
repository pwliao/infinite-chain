#ifndef INF_SERVER_NEIGHBOR_H
#define INF_SERVER_NEIGHBOR_H

#include <string>
#include <vector>

struct Neighbor {
   string ip;
   int port;
};

class Neighbors {
public:
    void broadcast(std::string message);
private:
    std::vector<Neighbor> neighbors
};

#endif //INF_SERVER_NEIGHBOR_H
