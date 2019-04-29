#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include <cstdint>
#include <string>
#include <map>
#include <leveldb/db.h>

struct BlockHeaders {
	uint32_t version;
	std::string previous_hash;
	std::string merkle_root_hash;
	std::string target;
	uint32_t nonce;
	BlockHeaders();
	BlockHeaders(uint32_t, std::string, std::string, std::string, uint32_t);
	BlockHeaders(std::string serialized_headers);
	std::string serialize();
	std::string hash();
};

struct Block {
	BlockHeaders headers;
	int height;
	Block() {}
	Block(std::string serialized_block);
	std::string getMerkleRoot();
	std::string serialize();
};

struct Blockchain {
	std::string target;
	leveldb::DB* db;
	leveldb::Status status;
	Blockchain() {}
	Blockchain(std::string target);
	int getBlockCount();
	std::string getBlockHash(int block_height);
	void addBlock(Block block);
	Block getBlock(std::string block_hash);
	void mining();
	void initDb();
};
#endif
