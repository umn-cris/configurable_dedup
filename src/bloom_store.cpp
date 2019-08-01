//
// Created by wyx on 2019/7/30.
//

#include "bloom_store.h"
bool bloom_partition::Append2Containers(container *cnr) {
    containers_.push_back(*cnr);
    cnr->reset();
    return true;
}
void bloom_partition::PartitionDedup(chunk ck) {
    local_total_chunks_++;
    cout<<local_total_chunks_<<" ";
    if(BFs_.empty()) {
        bloom_filter bf(888);
        BFs_.push_back(bf);
    }
    if(containers_.empty()){
        container cnr;
        containers_.push_back(cnr);
    }

    for(int i=0; i<BFs_.size(); i++){
        if(BFs_[i].BloomCheck(ck.ID().c_str())){
            if(i!=activeBF_) {
                local_IOtimes_++;
            }
            for(auto m:containers_[i].chunks_){
                if(ck.ID() == m.ID()){
                    return;
                }
            }
        }
    }
    local_stored_chunks_++;
    cout<<local_stored_chunks_<<endl;

    /*if(!containers_[activeBF_].AppendChunk(ck)){
        activeBF_++;
        container cnr;
        containers_.push_back(cnr);
        containers_[activeBF_].AppendChunk(ck);
        bloom_filter bf(888);
        BFs_.push_back(bf);
    }*/
    if(!current_cnr->AppendChunk(ck)){
        activeBF_++;
        Append2Containers(current_cnr);
        current_cnr->AppendChunk(ck);
        bloom_filter bf(888);
        BFs_.push_back(bf);
    }
    BFs_[activeBF_].BloomAdd(ck.ID().c_str());
}

long bloom_store::PartitionChunk(chunk ck) {
    return (hash_(ck.ID().c_str()) % g_cache_size);
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
        while (trace_ptr->HasNext()){
            chunk ck = trace_ptr->Next();
            long target = PartitionChunk(ck);
            cout<<"target: "<<target<<endl;
            partitions_[target].PartitionDedup(ck);
            cout<<"end target: "<<target<<endl;
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