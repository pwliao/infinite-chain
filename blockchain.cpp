#include "blockchain.hpp"
#include "transaction.hpp"
#include <cstdlib>
#include <cstdio>
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
						   string transactions_hash, string target, string beneficiary, uint32_t nonce)
{
	this->version = version;
	this->previous_hash = previous_hash;
	this->transactions_hash = transactions_hash;
	this->target = target;
	this->beneficiary = beneficiary;
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

	for (auto &tx_json : data["transactions"]) {
		Transaction tx(tx_json.dump());
		txs.push_back(tx);
	}
}

string Block::serialize()
{
	json serialized_block;
	serialized_block["data"] = json::object();
	serialized_block["data"] = {{"version", headers.version}, {"prev_block", headers.previous_hash},
		{"transactions_hash", headers.transactions_hash}, {"beneficiary", headers.beneficiary},
		{"target", headers.target}, {"nonce", headers.nonce}
	};
	serialized_block["data"]["transactions"] = json::array();
	serialized_block["height"] = height;
	for (auto &tx : txs) {
		json json_tx = json::parse(tx.serialize());
		serialized_block["data"]["transactions"].push_back(json_tx);
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
	if (headers.transactions_hash != calculateTransactionHash()) {
		return false;
	}
	if (headers.hash() > target) {
		return false;
	}
	return true;
}

bool Block::countWorldState(struct Blockchain &blockchain)
{
	Block previous_block = blockchain.getBlock(this->headers.previous_hash);
	map<string, uint64_t> world_state = previous_block.world_state;
	set<string> all_txs = previous_block.all_txs;

	cout << "==================== start count world state =========================" << endl;
	cout << "交易數量：" << this->txs.size() << endl;
	uint64_t all_fee = 0;
	for (auto &tx : this->txs) {
		if (tx.isValid() &&
				all_txs.find(tx.signature) == all_txs.end() &&
				world_state[tx.sender_pub_key] >= (tx.fee + tx.value)) {

			all_fee += tx.fee;
			world_state[tx.sender_pub_key] -= (tx.fee + tx.value);
			cout << tx.sender_pub_key << " 減少 " << (tx.fee + tx.value) << endl;
			world_state[tx.to] += tx.value;
			cout << tx.to << " 增加 " << (tx.value) << endl;

			all_txs.insert(tx.signature);
		} else {
			cout << "==================== fail count world state =========================" << endl;
			return false;
		}
	}
	// TODO: 將 1000 放到 config.json 或是其他專門放常數的檔案
	world_state[this->headers.beneficiary] += (1000 + all_fee);
	cout << this->headers.beneficiary << " 礦工獲得手續費 " << all_fee << " 以及挖礦獎勵 " << 1000 << endl;
	this->world_state = world_state;
	this->all_txs = all_txs;
	cout << "==================== end count world state =========================" << endl;
	return true;
}

string Block::calculateTransactionHash()
{
	string signature;
	for (auto &tx : txs) {
		signature += tx.signature;
	}
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, signature.c_str(), signature.size());
	SHA256_Final(hash, &sha256);
	stringstream ss;
	for (unsigned char i : hash) {
		ss << hex << setw(2) << setfill('0') << (int)i;
	}
	return ss.str();
}

Blockchain::Blockchain(json config)
{
	initDb();
	string zero = "0000000000000000000000000000000000000000000000000000000000000000";
	this->target = config["target"];
	this->beneficiary = config["beneficiary"];
	this->public_key = config["wallet"]["public_key"];
	this->private_key = config["wallet"]["private_key"];
	this->fee = config["fee"];
	this->neighbors = Neighbors();
	for (auto n : config["neighbor_list"]) {
		neighbors.addNeighbor({ n["ip"], n["p2p_port"] });
	}

	string latest_block;
	leveldb::Status status = db->Get(leveldb::ReadOptions(), "latest_block", &latest_block);
	if (!status.ok()) {
		db->Put(leveldb::WriteOptions(), "latest_block_hash", zero);
		db->Put(leveldb::WriteOptions(), "latest_block", "-1");
		db->Put(leveldb::WriteOptions(), "nonce", "0");
	}
	Block block;
	block.height = -1;
	block.headers = BlockHeaders(1, zero, zero, this->target, this->beneficiary, 0);
	saveBlock(zero, block);
}

int Blockchain::getBlockCount()
{
	string latest_block;
	db->Get(leveldb::ReadOptions(), "latest_block", &latest_block);
	return stoi(latest_block);
}

bool Blockchain::addBlock(Block block)
{
	lock_guard<std::mutex> lock(this->add_block_mutex);
	Block previous_block = getBlock(block.headers.previous_hash);
	string block_hash = block.headers.hash();
	int latest_block = getBlockCount();
	if (!block.countWorldState(*this)) {
		return false;
	}
	if (!block.isValid(this->target)) {
		printf("invalid block\n");
		return false;
	}
	if (block.height > latest_block) {
		db->Put(leveldb::WriteOptions(), "latest_block", to_string(block.height));
		db->Put(leveldb::WriteOptions(), "latest_block_hash", block.headers.hash());
	}
	saveBlock(block_hash, block);
	this->showWorldState();
	return true;
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
	this->neighbors.broadcast(message.dump());
}

void Blockchain::broadcastTransaction(Transaction tx)
{
	json message = json::object();
	message["data"] = json::parse(tx.serialize());
	message["method"] = "sendTransaction";
	this->neighbors.broadcast(message.dump());
}

void Blockchain::mining()
{
	uint32_t nonce = 0;
	while (true) {
		Block block;
		string latest_block_hash;
		db->Get(leveldb::ReadOptions(), "latest_block_hash", &latest_block_hash);
		Block previous_block = this->getBlock(latest_block_hash);
		block.headers = BlockHeaders(2, latest_block_hash,
									 "", target, beneficiary, nonce);
		block.height = previous_block.height + 1;

//		加入交易
		auto world_state = previous_block.world_state;
		auto all_txs = previous_block.all_txs;
		this->tx_pool_mutex.lock();
		for (Transaction tx : this->transaction_pool) {
			if (all_txs.find(tx.signature) == all_txs.end() &&
					world_state[tx.sender_pub_key] >= (tx.fee + tx.value)) {

				world_state[tx.sender_pub_key] -= (tx.fee + tx.value);
				world_state[tx.to] += tx.value;

				all_txs.insert(tx.signature);
				block.txs.push_back(tx);

			}
		}
		this->tx_pool_mutex.unlock();
		block.headers.transactions_hash = block.calculateTransactionHash();

		int T = 10000;
		while (T--) {
			block.headers.nonce = nonce;
			if (block.headers.hash() <= target) {
				cerr << "new block, height " << block.height << endl;
				this->broadcastBlock(block);
				this->addBlock(block);
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

unsigned int Blockchain::getBalance(std::string address)
{
	Block b = this->getLatestBlock();
	b = this->getBlock(b.headers.previous_hash);
	b = this->getBlock(b.headers.previous_hash);

	return b.world_state[address];
}

Block Blockchain::getLatestBlock()
{
	string latest_block_hash;
	db->Get(leveldb::ReadOptions(), "latest_block_hash", &latest_block_hash);
	return getBlock(latest_block_hash);
}

void Blockchain::showWorldState()
{
	Block b = this->getLatestBlock();
	b = this->getBlock(b.headers.previous_hash);
	b = this->getBlock(b.headers.previous_hash);

	cout << "show world state" << endl;
	auto world_state = b.world_state;
	for (auto s : world_state) {
		cout << "賬戶： " << s.first << ", 餘額: " << s.second << endl;
	}
}

void Blockchain::sendToAddress(std::string address, uint64_t amount)
{
	string nonce_str;
	db->Get(leveldb::ReadOptions(), "nonce", &nonce_str);
	int nonce = stoi(nonce_str);
	db->Put(leveldb::WriteOptions(), "nonce", to_string(nonce + 1));
	Transaction tx(nonce, this->public_key, address,
				   amount, this->fee);
	tx.sign(this->private_key);
	this->broadcastTransaction(tx);
	this->tx_pool_mutex.lock();
	this->transaction_pool.push_back(tx);
	this->tx_pool_mutex.unlock();
}

bool Blockchain::addRemoteTransaction(std::string tx_str)
{
	Transaction tx(tx_str);
	if (tx.isValid()) {
		this->tx_pool_mutex.lock();
		this->transaction_pool.push_back(tx);
		this->tx_pool_mutex.unlock();
		return true;
	} else {
		return false;
	}
}
