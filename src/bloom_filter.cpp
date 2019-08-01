//
// Created by wyx on 2019/7/30.
//

#include "bloom_filter.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include<limits.h>
#include<stdarg.h>



unsigned int bloom_filter::sax_hash(const char *key)
{
    unsigned int h=0;

    while(*key) h^=(h<<5)+(h>>2)+(unsigned char)*key++;

    return h;
}

unsigned int bloom_filter::sdbm_hash(const char *key)
{
    unsigned int h=0;
    while(*key) h=(unsigned char)*key++ + (h<<6) + (h<<16) - h;
    return h;
}



#define SETBIT(a, n) (a[n/CHAR_BIT] |= (1<<(n%CHAR_BIT)))
#define GETBIT(a, n) (a[n/CHAR_BIT] & (1<<(n%CHAR_BIT)))

bloom_filter::bloom_filter(size_t size) {
    bitmap_= static_cast<unsigned char *>(calloc((size + CHAR_BIT - 1) / CHAR_BIT, sizeof(char)));
    bfsize_ = size;
}

bloom_filter::~bloom_filter()
{
    free(bitmap_);
    return;
}


void bloom_filter::BloomReload(size_t size)
{
    if (bfsize_ != size) {
        free(bitmap_);
        bitmap_= static_cast<unsigned char *>(calloc((size + CHAR_BIT - 1) / CHAR_BIT, sizeof(char)));
        bfsize_ = size;
    }
    return;
}


bool bloom_filter::BloomAdd(const char *s)
{
    if (s == nullptr) {
        return false;
    }
    SETBIT(bitmap_, sax_hash(s)%bfsize_);
    SETBIT(bitmap_, sdbm_hash(s)%bfsize_);
    return true;
}

bool bloom_filter::BloomCheck(const char *s)
{
    size_t n;

    if(!(GETBIT(bitmap_, sax_hash(s)%bfsize_))) {
        return false;
    }
    if(!(GETBIT(bitmap_, sdbm_hash(s)%bfsize_))) {
        return false;
    }

    return true;
}


