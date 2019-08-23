//
// Created by wyx on 2019/7/30.
//

#include "bloom_store.h"

void bloom_partition::PartitionDedup(chunk ck) {
    local_total_chunks_++;
    //cout<<local_total_chunks_<<" ";

    if(BFs_.empty()) {
        BloomFilter<string> bf(g_container_size,0.1);
        BFs_.push_back(bf);
    }
    if(containers_.empty()){
        container cnr;
        containers_.push_back(cnr);
    }

    for(int i=0; i<BFs_.size(); i++){
        if(BFs_[i].key_may_match(ck.ID())){
            if(activeBF_.find(i)==activeBF_.end()) {
                local_IOtimes_++;
                if(BFlist_.size()>=g_BFcache_size){
                    activeBF_.erase(BFlist_.back());
                    BFlist_.pop_back();
                }
                BFlist_.push_front(i);
                activeBF_.emplace(i);
            }
             else{
                 BFlist_.remove(i);
                 BFlist_.push_front(i);
            }
        }
    }
		if (cache_.find(ck.ID()) != cache_.end()) {
			return;
		}
    local_stored_chunks_++;
    //cout<<local_stored_chunks_<<endl;
    if(!containers_[curBF_].AppendChunk(ck)){
        curBF_++;
        container cnr;
        containers_.push_back(cnr);
        containers_[curBF_].AppendChunk(ck);
        BloomFilter<string> bf(g_container_size,0.1);
        BFs_.push_back(bf);
    }
    cache_.insert(ck.ID());
    BFs_[curBF_].insert(ck.ID());
}

long bloom_store::PartitionChunk(chunk ck) {
    return (hash_(ck.ID().c_str()) % g_partition_number);
}

void bloom_store::DoDedup() {
    string trace_sum, trace_name, trace_line;
    ifstream TraceSumFile;
    trace_sum = g_dedup_trace_dir + g_trace_summary_file;
    TraceSumFile.open(trace_sum);
    if (TraceSumFile.fail()) {
        cerr << "open "<< trace_sum <<  "failed!\n";
        exit(1);
    }

    while(getline(TraceSumFile, trace_line)) {
        long last_IOloads=0, IOloads=0;
        long last_total_chunks=0, total_chunks=0;
        long last_stored_chunks=0, stored_chunks=0;
        for(auto n:partitions_){
            last_total_chunks+=n.local_total_chunks_;
            last_stored_chunks+=n.local_stored_chunks_;
            last_IOloads+=n.local_IOtimes_;
        }
        stringstream ss(trace_line);
        getline(ss, trace_name, ' ');
        string trace_path;
        trace_path = g_dedup_trace_dir + trace_name;
        TraceReader *trace_ptr = new TraceReader(trace_path);
        chunk ck;
        while (trace_ptr->HasNext()){
             ck = trace_ptr->Next();
            long target = PartitionChunk(ck);
            //cout<<"target: "<<target<<endl;
            partitions_[target].PartitionDedup(ck);
            //cout<<"end target: "<<target<<endl;
        }
        for(auto n:partitions_){
            total_chunks+=n.local_total_chunks_;
            stored_chunks+=n.local_stored_chunks_;
            IOloads+=n.local_IOtimes_;
        }
        long current_IOloads = IOloads - last_IOloads;
        long current_total_chunks = total_chunks - last_total_chunks;
        long current_stored_chunks = stored_chunks - last_stored_chunks;
        double current_deduprate = current_total_chunks/(current_stored_chunks*1.0);
        double overall_deduprate = total_chunks/(stored_chunks*1.0);
        cout<<"current IOloads:"<<current_IOloads<<" overall IOloads:"<<IOloads<<endl;
        cout<<"current deduprate:"<<current_deduprate<<" overall deduprate:"<<overall_deduprate<<endl<<endl;
    }
}
