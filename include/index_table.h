//
// Created by wyx on 19-6-20.
//

#include <set>
#include "global.h"
#include "subset.h"
#include "sample_handler.h"
#ifndef CONFIGURABLE_DEDUP_INDEX_TABLE_H
#define CONFIGURABLE_DEDUP_INDEX_TABLE_H

extern vector<container> containers_;
extern vector<recipe> recipes_;

class index_table{
public:
    virtual bool LookUp(chunk& ck)=0;
    virtual void EraseChunk(chunk ck)=0;
};

class hook_entry{
public:
    hook_entry(){}
    list<meta_data> candidates_; //subset score & subset's metadata
};

class hook_table: public index_table{
private:
public:
    unordered_map<string,hook_entry> cached_hooks_;

    bool LookUp(chunk& ck){
        if(cached_hooks_.find(ck.ID())!=cached_hooks_.end())
            return true;
        else
            return false;
    }

    bool LookUp(string id, hook_entry** entry){
        if(cached_hooks_.find(id)!=cached_hooks_.end()){
            *entry = &cached_hooks_[id];
            return true;
        }
        else
            return false;
    }

    list<meta_data> LookUp(string id){
        if(cached_hooks_.find(id)!=cached_hooks_.end()){
            return cached_hooks_[id].candidates_;
        } else cout<<"[index_table.h] [LookUp(string id)] no candidates found"<<endl;
    }

    void InsertFeatures(const chunk& cks, meta_data meta);



    void EraseChunk(chunk ck){
        EraseHookTable(ck);
    }

    list<meta_data> SelectFIFO(const list<chunk>& features);
    list<meta_data> SelectLevel(list<meta_data> recipe_cds_list);
    list<meta_data> SelectSparse(unordered_map<long, set<string>> cds_map);
    list<meta_data> SelectSort(list<meta_data> cds_list);
    list<meta_data> PickCandidates(const list<chunk>& features);
    list<meta_data> SimplePickCandidates(const list<chunk>& features);
    void EraseHookTable(chunk ck);
    void PrintHookInfo(){
        cout<<"print hook info~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl;
        cout<<"hook size:"<<cached_hooks_.size()<<endl;
        for(auto n:cached_hooks_) {
            cout<<n.first<<endl;
            for (auto m:n.second.candidates_){
                cout<<"     name:"<<m.Name()<<" cnr?"<<m.IfCnr()<<" score"<<m.Score()<<endl;
            }
        }
        cout<<endl;
    }
};

class lru_cache: public index_table{
private:
    long capacity_ ;
    unordered_map<string,chunk> cached_chunks_;
    list<meta_data> lru_cache_ ;
public:
    lru_cache() {
        capacity_= g_cache_size;
    }

    bool LookUp(chunk& ck){
        if(cached_chunks_.find(ck.ID())!=cached_chunks_.end()){
            //ck = map_[ck.ID()];
            ck.SetLocation(cached_chunks_[ck.ID()].GetLocation());
            return true;
        }
        else
            return false;
    }

    bool LookUp(string id, chunk* ck){
        if(cached_chunks_.find(id)!=cached_chunks_.end()){
            *ck=cached_chunks_[id];
            return true;
        }
        else
            return false;
    }

    void InsertChunks(const list<chunk>& cks, bool if_cnr){
        for(auto n:cks){
            n.Cnr_or_Recipe(if_cnr);
            if(LookUp(n)){
                cached_chunks_[n.ID()] = n;
            }
            else{
                cached_chunks_.emplace(n.ID(), n);
            }
        }
    }

    void EraseChunk(chunk ck){
        cached_chunks_.erase(ck.ID());
    }

    bool Load(const meta_data value);
    void Evict();
    void Flush();
};
#endif //CONFIGURABLE_DEDUP_INDEX_TABLE_H
