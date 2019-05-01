#include "blockchain.hpp"
#include "transaction.hpp"
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <openssl/sha.h>
#include <leveldb/db.h>
#include "json.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

using json = nlohmann::json;
using namespace std;

BlockHeaders::BlockHeaders()
{
}

BlockHeaders::BlockHeaders(string serialized_headers)
{
	this->version = stoi(serialized_headers.substr(0, 8), nullptr, 16);
	this->previous_hash = serialized_headers.substr(8, 64);
	this->transactions_hash = serialized_headers.substr(72, 64);
	this->target = serialized_headers.substr(136, 64);
	this->nonce = stoi(serialized_headers.substr(200, 8), nullptr, 16);
}

BlockHeaders::BlockHeaders(uint32_t version, string previous_hash,
		string transactions_hash, string target, uint32_t nonce)
{
	this->version = version;
	this->previous_hash = previous_hash;
	this->transactions_hash = transactions_hash;
	this->target = target;
	this->nonce = nonce;
}

string BlockHeaders::serialize()
{
	stringstream ss;
	ss << hex << setw(8) << setfill('0') << version;
	ss << previous_hash;
	ss << transactions_hash;
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
	for (unsigned char i : hash) {
		ss << hex << setw(2) << setfill('0') << (int)i;
	}
	return ss.str();
}

Block::Block(string serialized_block)
{
	json j = json::parse(serialized_block);
	json data = j["data"];
	headers.version = data["version"];
	headers.previous_hash = data["prev_block"];
	headers.transactions_hash = data["transactions_hash"];
	headers.beneficiary = data["beneficiary"];
	headers.target = data["target"];
	headers.nonce = data["nonce"];
	height = j["height"];

	for (auto &tx_json: data["transactions"]) {
	    Transaction tx(tx_json.dump());
	    txs.push_back(tx);
	}
}

string Block::getMerkleRoot()
{
	return "0000000000000000000000000000000000000000000000000000000000000000";
}

string Block::serialize()
{
	json serialized_block;
	serialized_block["data"] = json::object();
	serialized_block["data"] = {{"version", headers.version}, {"prev_block", headers.previous_hash},
		{"transactions_hash", headers.transactions_hash}, {"beneficiary", headers.beneficiary},
		{"target", headers.target}, {"nonce", headers.nonce}};
	serialized_block["data"]["transaction"] = json::array();
	serialized_block["height"] = height;
	for (auto &tx : txs) {
		serialized_block["data"]["transaction"].push_back(tx.serialize());
	}
	return serialized_block.dump();
}

bool Block::isValid(const string &target)
{
	if (headers.version != 2) {
		return false;
	}
	if (headers.target != target) {
		return false;
	}
	// TODO check transactions
	if (headers.hash() > target) {
		return false;
	}
	return true;
}

bool Block::countWorldState(struct Blockchain &blockchain) {
//    Block previous_block = blockchain.getBlock(this->headers.previous_hash);
//    map<string, uint64_t> world_state =
//    for (auto &tx: this->txs) {
//
//    }
    return false;
}

Blockchain::Blockchain(string target)
{
	initDb();
	string zero = "0000000000000000000000000000000000000000000000000000000000000000";
	this->target = target;
	string latest_block;
	leveldb::Status status = db->Get(leveldb::ReadOptions(), "latest_block", &latest_block);
	if (!status.ok()) {
		db->Put(leveldb::WriteOptions(), "latest_block_hash", zero);
		db->Put(leveldb::WriteOptions(), "latest_block", "-1");
	}
	Block block;
	block.height = -1;
	block.headers = BlockHeaders(1, zero, zero, target, 0);
	saveBlock(zero, block);
}

int Blockchain::getBlockCount()
{
	string latest_block;
	db->Get(leveldb::ReadOptions(), "latest_block", &latest_block);
	return stoi(latest_block);
}

void Blockchain::addBlock(Block block)
{
	Block previous_block = getBlock(block.headers.previous_hash);
	string block_hash = block.headers.hash();
	int latest_block = getBlockCount();
	if (block.height > latest_block) {
		db->Put(leveldb::WriteOptions(), "latest_block", to_string(block.height));
		db->Put(leveldb::WriteOptions(), "latest_block_hash", block.headers.hash());
	}
	saveBlock(block_hash, block);
}

Block Blockchain::getBlock(string block_hash)
{
	string serialized_block;
	leveldb::Status s = db->Get(leveldb::ReadOptions(), block_hash, &serialized_block);
	Block block;
	istringstream is(serialized_block);
	boost::archive::binary_iarchive ia(is);
	ia >> block;
	return block;
}

void Blockchain::saveBlock(string block_hash, Block block)
{
	ostringstream os;
	boost::archive::binary_oarchive oa(os);
	oa << block;
	db->Put(leveldb::WriteOptions(), block_hash, os.str());
}

void Blockchain::broadcastBlock(Block block)
{
	json message = json::parse(block.serialize());
	message["method"] = "sendBlock";
	neighbors.broadcast(message.dump());
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
		int T = 10000;
		while (T--) {
			block.headers.nonce = nonce;
			if (block.headers.hash() <= target) {
				cerr << "new block" << endl;
				addBlock(block);
				break;
			}
			nonce++;
		}
	}
}

void Blockchain::initDb()
{
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, "db", &db);
	assert(status.ok());
}

unsigned int Blockchain::getBalance(std::string address) {
    return 0;
}

Block Blockchain::getLatestBlock()
{
	string latest_block_hash;
	db->Get(leveldb::ReadOptions(), "latest_block_hash", &latest_block_hash);
	return getBlock(latest_block_hash);
}
