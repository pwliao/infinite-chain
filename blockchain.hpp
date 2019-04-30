#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "transaction.hpp"
#include <leveldb/db.h>

struct BlockHeaders {
	uint32_t version;
	std::string previous_hash;
	std::string merkle_root_hash;
	std::string target;
	std::string beneficiary;
	uint32_t nonce;
	BlockHeaders();
	BlockHeaders(uint32_t, std::string, std::string, std::string, uint32_t);
	BlockHeaders(std::string serialized_headers);
	std::string serialize();
	std::string hash();
};

struct Block {
	Block() {}
	Block(std::string serialized_block);
	std::string getMerkleRoot();
	std::string serialize();
	bool isValid(const std::string &target);

	BlockHeaders headers;
	int height;
	std::vector<Transaction> txs;
};

struct Blockchain {
	std::string target;
	leveldb::DB* db;
	Blockchain() {}
	Blockchain(std::string target);
	int getBlockCount();
	void addBlock(Block block);
	Block getBlock(std::string block_hash);
	void mining();
	void initDb();
	unsigned int getBalance(std::string address);
};
#endif
