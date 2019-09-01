//
// Created by wyx on 19-6-20.
//

#ifndef CONFIGURABLE_DEDUP_GLOBAL_H
#define CONFIGURABLE_DEDUP_GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <ctime>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <list>
#include <sstream>
#include <time.h>
#include <deque>
#include <algorithm>
#include <fstream>
#include <cassert>
#include <bitset>
#include <utility>
#include <math.h>
using namespace std;
extern bool g_if_exact;
extern bool g_if_hybrid;
extern bool g_only_cnr;
extern bool g_only_recipe;
extern bool g_if_flush;
extern long g_cache_size;
extern long g_chunk_size;
extern long g_window_size;
extern long g_container_size;
extern long g_IO_cap;
extern string g_dedup_trace_dir;
extern string g_trace_summary_file;
extern long g_bit_num1;
extern long g_bit_num2;
extern long g_segment_size;
extern long g_random_pick_ratio;
extern long IOloads;
extern long cnr_IOloads;
extern long recipe_IOloads;
extern long g_segmenting_bit_num;
extern bool g_debug_output;
extern long g_dedup_engine_no;
extern long g_BFcache_size;
extern long g_partition_number;
extern long g_recipe_version_bound;
extern string g_selection_policy;

int Parse(string cfgfile);

typedef std::uint64_t hash_t;
constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;
constexpr hash_t hash_(char const* str, hash_t last_value = basis)
{
    return *str ? hash_(str+1, (*str ^ last_value) * prime) : last_value;
}
#endif //CONFIGURABLE_DEDUP_GLOBAL_H
