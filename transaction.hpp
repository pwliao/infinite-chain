#ifndef TRANSACTION
#define TRANSACTION

#include <cstdint>
#include <string>

class Transaction
{
public:
	Transaction() = default;
	Transaction(uint64_t nonce, std::string sender_pub_key, std::string to, uint64_t value, uint64_t fee)
		: nonce(nonce), sender_pub_key(sender_pub_key), to(to), value(value), fee(fee) {}
	Transaction(std::string serialized_transaction);
	std::string serialize();
	std::string sign(std::string private_key); // 其它欄位都搞定的狀況下填入 signature
private:
	uint64_t nonce;
	std::string sender_pub_key;
	std::string to;
	uint64_t value;
	uint64_t fee;
	std::string signature;
};
#endif
