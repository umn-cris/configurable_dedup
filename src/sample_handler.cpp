//
// Created by wyx on 19-6-22.
//
#include "subset.h"
#include "sample_handler.h"

//2^-1 - 2^-4 = 2^-2+2^-3+2^-4 = 0.25+0.125+0.0625
bool sample_handler::Fromend_NegativeFeatures(const chunk& ck, long bit_num) {
    if(bit_num==0){
        return false;
    }
    bitset<48> bits(stoll(ck.ID(), nullptr, 16));
    for (int j = bits.size(); j > bits.size()-bit_num; --j) {
        if(bits[j]) {
            return false;
        }
    }
    return true;
}
bool sample_handler::Fromend_PositiveFeatures(const chunk& ck, long bit_num) {
    if(bit_num==0){
        return false;
    }
    bitset<48> bits(stoll(ck.ID(), nullptr, 16));
    for (int j = bits.size(); j > bits.size()-bit_num; --j) {
        if(!bits[j]) {
            return false;
        }
    }
    return true;
}
bool sample_handler::NegativeFeatures(const chunk& ck, long bit_num) {
    if(bit_num==0){
        return false;
    }
    bool is_hook = true;
    bitset<48> bits(stoll(ck.ID(), nullptr, 16));
    for (int j = 0; j < bit_num; ++j) {
        if(bits[j]) {
            is_hook= false;
            break;
        }
    }
    if(is_hook) return true;
    else return false;

}
bool sample_handler::Segmenting(const chunk& ck, long bit_num) {
    if(bit_num==0){
        return false;
    }
    bitset<48> bits(stoll(ck.ID(), nullptr, 16));
    for (int j = 0; j < bit_num; ++j) {
        if(!bits[j]) {
            return false;
        }
    }
    for (int j = bit_num; j < bit_num+bit_num; ++j) {
        if(bits[j]) {
            return false;
        }
    }

}
bool sample_handler::PositiveFeatures(const chunk& ck, long bit_num) {
    if(bit_num==0){
        return false;
    }
    bool is_hook = true;
    bitset<48> bits(stoll(ck.ID(), nullptr, 16));
    for (int j = 0; j < bit_num; ++j) {
        if(!bits[j]) {
            is_hook= false;
            break;
        }
    }
    if(is_hook) return true;
    else return false;
}

bool sample_handler::RandomPickFeature(const chunk &ck, long sample_ratio) {
    if(sample_gap_>=sample_ratio){
        sample_gap_=0;
        return true;
    }
    sample_gap_++;
    return false;
}

bool sample_handler::ChunkIDPickFeature(const chunk &ck, long sample_ratio) {
    string ss = ck.ID().substr(15,2);
    auto value = stol(ss, nullptr, 16);

    if ((value+1)%sample_ratio==0) return true;
    else return false;
}

bool sample_handler::SequentialPickFeature(const chunk &ck, long position) {
    if(position>0){
        return true;
    }
    return false;
}

bool sample_handler::MinFeatures(const chunk &ck, long bit_num) {

}