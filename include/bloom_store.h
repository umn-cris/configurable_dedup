//
// Created by wyx on 2019/7/30.
//

#ifndef CONFIGURABLE_DEDUP_BLOOM_STORE_H
#define CONFIGURABLE_DEDUP_BLOOM_STORE_H

#include "bloom_filter.h"
#include "subset.h"
#include "trace_reader.h"

class bloom_partition{
private:
    vector<bloom_filter> BFs_;
    vector<container> containers_;
    long activeBF_=0;
    container* current_cnr = new container(0,0,"container");

public:

    long local_total_chunks_=0;
    long local_stored_chunks_=0;
    long local_IOtimes_=0;
    void PartitionDedup(chunk ck);
    bool Append2Containers(container* cnr);

};
class bloom_store{
private:
    vector<bloom_partition> partitions_;//partition number = cache size

public:
    bloom_store(){
        for(int i=0; i<g_cache_size; i++){
            bloom_partition par;
            partitions_.push_back(par);
        }
    }
    long PartitionChunk(chunk ck);
    void DoDedup();
};
#endif //CONFIGURABLE_DEDUP_BLOOM_STORE_H
