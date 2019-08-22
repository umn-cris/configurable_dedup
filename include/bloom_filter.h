//
// Created by wyx on 2019/7/30.
//

#ifndef CONFIGURABLE_DEDUP_BLOOM_FILTER_H
#define CONFIGURABLE_DEDUP_BLOOM_FILTER_H

#include "global.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <string>

inline uint32_t decode_fixed32(const char* ptr) {
    // Load the raw bytes
    uint32_t result;
    memcpy(&result, ptr, sizeof(result));  // gcc optimizes this to a plain load
    return result;
}




template<typename T>
class BloomFilter
{
public:
    BloomFilter(const int32_t n, const double false_positive_p)
            : bits_(), k_(0), m_(0), n_(n), p_(false_positive_p)
    {
        k_ = static_cast<int32_t>(-std::log(p_) / std::log(2));
        m_ = static_cast<int32_t>(k_ * n * 1.0 / std::log(2));
        bits_.resize((m_ + 7) / 8, 0);
    }

    uint32_t hash_func(const char* data, size_t n, uint32_t seed) {
        // Similar to murmur hash
        const uint32_t m = 0xc6a4a793;
        const uint32_t r = 24;
        const char* limit = data + n;
        uint32_t h = seed ^ (n * m);
        // Pick up four bytes at a time
        while (data + 4 <= limit) {
            uint32_t w = decode_fixed32(data);
            data += 4;
            h += w;
            h *= m;
            h ^= (h >> 16);
        }


        // Pick up remaining bytes
        switch (limit - data) {
            case 3:
                h += static_cast<unsigned char>(data[2]) << 16;
            case 2:
                h += static_cast<unsigned char>(data[1]) << 8;
            case 1:
                h += static_cast<unsigned char>(data[0]);
                h *= m;
                h ^= (h >> r);
                break;
        }
        return h;
    }

    void insert(const T &key)
    {
        uint32_t hash_val = 0xbc9f1d34;
        for (int i = 0; i < k_; ++i) {
            hash_val = hash_func(key.data(),key.size(),hash_val);
            const uint32_t bit_pos = hash_val % m_;
            bits_[bit_pos/8] |= 1 << (bit_pos % 8);
        }
    }
    bool key_may_match(const T &key)
    {
        uint32_t hash_val = 0xbc9f1d34;
        for (int i = 0; i < k_; ++i) {
            hash_val = hash_func(key.data(),key.size(),hash_val);
            const uint32_t bit_pos = hash_val % m_;
            if ((bits_[bit_pos/8] & (1 << (bit_pos % 8))) == 0) {
                return false;
            }
        }
        return true;
    }

private:
    vector<char> bits_;
    int32_t k_;
    int32_t m_;
    int32_t n_;
    double p_;
};
#endif //CONFIGURABLE_DEDUP_BLOOM_FILTER_H
