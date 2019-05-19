CXX = g++
CPPFLAGS = -O3 -std=c++11
LIBS = -lpthread -lcrypto -lleveldb -lboost_serialization

all: main.o api.o blockchain.o transaction.o neighbor.o ecc/uECC.o
	$(CXX) $(CPPFLAGS) $(LIBS) $^ -o infinite-chain

%.o : %.c
	$(CXX) $(CPPFLAGS) -c $< -o $@

%.o : %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

run:
	./infinite-chain

clean:
	rm -rf db
