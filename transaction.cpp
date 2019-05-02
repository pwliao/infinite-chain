#include "transaction.hpp"
#include <openssl/sha.h>
#include "ecc/uECC.h"
#include <cstdint>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

const struct uECC_Curve_t* curve = uECC_secp256k1();

uint8_t trans_hex(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        throw "不是 hex";
    }
}

void hex_to_char(string hex, uint8_t *cs, uint32_t length) {
    for (auto i = 0; i < length; i++) {
        cs[i] = (trans_hex(hex[2 * i]) << 4) + trans_hex(hex[2 * i + 1]);
    }
}

string uint64_to_string(uint64_t x) {
    stringstream ss;
    ss << hex << setw(16) << setfill('0') << x;
    return ss.str();
}

void Transaction::partial_hash(unsigned char *hash) {
    string serialized = uint64_to_string(this->nonce)
                        + this->sender_pub_key
                        + this->to
                        + uint64_to_string(this->value)
                        + uint64_to_string(this->fee);

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, serialized.c_str(), serialized.size());
    SHA256_Final(hash, &sha256);
}

string Transaction::serialize() {

}

string Transaction::sign(std::string private_key) {


    unsigned char hash[SHA256_DIGEST_LENGTH];
    this->partial_hash(hash);

    uint8_t pri_key[32] = {0};
    uint8_t pub_key[64] = {0};
    uint8_t sig[64] = {0};

    hex_to_char(private_key, pri_key, 32);
    hex_to_char(this->sender_pub_key, pub_key, 64);

    uECC_sign(pri_key, hash, sizeof(hash), sig, curve);

    stringstream ret;
    for (int i = 0; i < 64; i++) {
        ret << hex << setw(2) << setfill('0') << (int)sig[i];
    }

    this->signature = ret.str();

    return ret.str();
}

bool Transaction::isValid() {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    this->partial_hash(hash);

    uint8_t pub_key[64] = {0};
    uint8_t sig[64] = {0};

    hex_to_char(this->sender_pub_key, pub_key, 64);
    hex_to_char(this->signature, sig, 64);

    return uECC_verify(pub_key, hash, sizeof(hash), sig, curve);
}

Transaction::Transaction(std::string serialized_transaction) {
    json tx = json::parse(serialized_transaction);
    this->nonce = tx["nonce"];
    this->sender_pub_key = tx["sender_pub_key"];
    this->to = tx["to"];
    this->value = tx["value"];
    this->fee = tx["fee"];
    this->signature = tx["signature"];
}
