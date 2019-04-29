#include "blockchain.hpp"
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <openssl/sha.h>
#include <leveldb/db.h>

using namespace std;

BlockHeaders::BlockHeaders()
{
}

BlockHeaders::BlockHeaders(string serialized_headers)
{
	this->version = stoi(serialized_headers.substr(0, 8), nullptr, 16);
	this->previous_hash = serialized_headers.substr(8, 64);
	this->merkle_root_hash = serialized_headers.substr(72, 64);
	this->target = serialized_headers.substr(136, 64);
	this->nonce = stoi(serialized_headers.substr(200, 8), nullptr, 16);
}

BlockHeaders::BlockHeaders(uint32_t version, string previous_hash,
		string merkle_root_hash, string target, uint32_t nonce)
{
	this->version = version;
	this->previous_hash = previous_hash;
	this->merkle_root_hash = merkle_root_hash;
	this->target = target;
	this->nonce = nonce;
}

string BlockHeaders::serialize()
{
	stringstream ss;
	ss << hex << setw(8) << setfill('0') << version;
	ss << previous_hash;
	ss << merkle_root_hash;
	ss << target;
	ss << hex << setw(8) << setfill('0') << nonce;
	return ss.str();
}

string BlockHeaders::hash()
{
	string str = serialize();
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, str.c_str(), str.size());
	SHA256_Final(hash, &sha256);
	stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		ss << hex << setw(2) << setfill('0') << (int)hash[i];
	}
	return ss.str();
}

Block::Block(string serialized_block)
{
	string headers_str;
	stringstream ss(serialized_block);
	ss >> headers_str >> height;
	headers = BlockHeaders(headers_str);
}

string Block::getMerkleRoot()
{
	return "0000000000000000000000000000000000000000000000000000000000000000";
}

string Block::serialize()
{
	return headers.serialize() + " " + to_string(height);
}

Blockchain::Blockchain(string target)
{
	initDb();
	string zero = "0000000000000000000000000000000000000000000000000000000000000000";
	this->target = target;
	db->Put(leveldb::WriteOptions(), "latest_block_hash", zero);
	db->Put(leveldb::WriteOptions(), "latest_block", "-1");
	Block block;
	block.height = -1;
	block.headers = BlockHeaders(1, zero, zero, target, 0);
	db->Put(leveldb::WriteOptions(), zero, block.serialize());
}

int Blockchain::getBlockCount()
{
	string latest_block;
	db->Get(leveldb::ReadOptions(), "block_hash", &latest_block);
	return stoi(latest_block);
}

string Blockchain::getBlockHash(int block_height)
{
	return "0";
}

void Blockchain::addBlock(Block block)
{
	Block previous_block = getBlock(block.headers.previous_hash);
	string block_hash = block.headers.hash();
	int latest_block = getBlockCount();
	if (block.height > latest_block) {
		db->Put(leveldb::WriteOptions(), "latest_block", to_string(block.height));
		db->Put(leveldb::WriteOptions(), "latest_block_hash", to_string(block.height));
	}
	db->Put(leveldb::WriteOptions(), block_hash, block.serialize());
}

Block Blockchain::getBlock(string block_hash)
{
	string serialized_block;
	leveldb::Status s = db->Get(leveldb::ReadOptions(), block_hash, &serialized_block);
	return Block(serialized_block);
}

void Blockchain::mining()
{
	uint32_t nonce = 0;
	while (true) {
		Block block;
		string latest_block_hash;
		db->Get(leveldb::ReadOptions(), "latest_block_hash", &latest_block_hash);
		block.headers = BlockHeaders(1, latest_block_hash,
				block.getMerkleRoot(), target, nonce);
		block.height = getBlock(block.headers.previous_hash).height + 1;
		if (block.headers.hash() <= target) {
			cerr << "new block" << endl;
			addBlock(block);
		}
		nonce++;
	}
}

void Blockchain::initDb()
{
	leveldb::Options options;
	options.create_if_missing = true;
	status = leveldb::DB::Open(options, "db", &db);
	assert(status.ok());
}
