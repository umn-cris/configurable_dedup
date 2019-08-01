//
// Created by wyx on 2019/7/30.
//

#ifndef CONFIGURABLE_DEDUP_BLOOM_FILTER_H
#define CONFIGURABLE_DEDUP_BLOOM_FILTER_H

#include "global.h"

typedef unsigned int (*hashfunc_t)(const char *);

class bloom_filter {
private:
    size_t bfsize_;
    unsigned char *bitmap_;

    unsigned int sax_hash(const char *key);
    unsigned int sdbm_hash(const char *key);
public:
    bloom_filter(size_t size);
    ~bloom_filter();
    void BloomReload(size_t);
    bool BloomAdd(const char *s);
    bool BloomCheck(const char *s);

};
#endif //CONFIGURABLE_DEDUP_BLOOM_FILTER_H
