#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "transaction.hpp"
#include "neighbor.hpp"
#include <leveldb/db.h>
#include <map>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include "json.hpp"

struct BlockHeaders {
	uint32_t version;
	std::string previous_hash;
	std::string transactions_hash;
	std::string target;
	std::string beneficiary;
	uint32_t nonce;
	BlockHeaders();
    BlockHeaders(uint32_t version
            , std::string previous_hash
            , std::string transactions_hash
            , std::string target, std::string beneficiay
            , uint32_t);
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
	bool countWorldState(struct Blockchain &blockchain);

	BlockHeaders headers;
	int height;
	std::vector<Transaction> txs;
    std::map<std::string, uint64_t> world_state;
	template<class Archive>
	void serialize(Archive& archive, unsigned int version)
	{
		archive & headers.version;
		archive & headers.previous_hash;
		archive & headers.transactions_hash;
		archive & headers.target;
		archive & headers.beneficiary;
		archive & headers.nonce;
		archive & height;
		archive & txs;
		archive & world_state;
	}
};

struct Blockchain {
	std::string target;
	std::string beneficiary;
	leveldb::DB* db;
	Neighbors neighbors;
	std::vector<Transaction> transaction_pool;

	Blockchain() {}
	Blockchain(nlohmann::json config);

	int getBlockCount();
	void broadcastBlock(Block block);
	void addBlock(Block block);
	void saveBlock(std::string hash, Block block);
	Block getBlock(std::string block_hash);
	void mining();
	void initDb();
	unsigned int getBalance(std::string address);
	void showWorldState();
	Block getLatestBlock();
};
#endif
