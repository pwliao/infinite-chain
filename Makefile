all:
	g++ -O3 main.cpp rpc.cpp blockchain.cpp -std=c++11 -lpthread -lcrypto -lleveldb -o infinite-chain

run:
	./infinite-chain
