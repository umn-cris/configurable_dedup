//
// Created by wyx on 19-6-20.
//
#include "global.h"
#include "subset.h"
#include "sample_handler.h"
#ifndef CONFIGURABLE_DEDUP_INDEX_TABLE_H
#define CONFIGURABLE_DEDUP_INDEX_TABLE_H



class index_table{
public:
    virtual bool LookUp(chunk ck)=0;
    virtual void EraseChunk(chunk ck)=0;
};

class hook_entry{
public:
    chunk ck_;
    list<meta_data> candidates_; //subset score & subset's metadata

};

class hook_table: public index_table{
private:
public:
    unordered_map<string,hook_entry> map_;

    bool LookUp(chunk ck){
        if(map_.find(ck.ID())!=map_.end())
            return true;
        else
            return false;
    }

    bool LookUp(string id, hook_entry** entry){
        if(map_.find(id)!=map_.end()){
            *entry = &map_[id];
            return true;
        }
        else
            return false;
    }

    list<meta_data> LookUp(string id){
        if(map_.find(id)!=map_.end()){
            return map_[id].candidates_;
        }
    }

    void InsertRecipeFeatures(const list<chunk>& cks);
    void InsertCnrFeatures(const list<chunk>& cks);
    void Leveling(list<pair<long, list<meta_data>>>& level_sort, list<meta_data>& recipe_cds_list);

    void EraseChunk(chunk ck){
        EraseHookTable(ck);
    }

    list<meta_data> PickCandidates(const list<chunk>& features);
    list<meta_data> PickCandidatesFIFO(const list<chunk>& features);

        void EraseHookTable(chunk ck);
    void PrintHookInfo(){
        cout<<"print hook info~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl;
        cout<<"hook size:"<<map_.size()<<endl;
        for(auto n:map_) {
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
    unordered_map<string,chunk> map_;
    list<meta_data> lru_cache_ ;
public:
    lru_cache() {
        capacity_= g_cache_size;
    }

    bool LookUp(chunk ck){
        if(map_.find(ck.ID())!=map_.end())
            return true;
        else
            return false;
    }

    bool LookUp(string id, chunk* ck){
        if(map_.find(id)!=map_.end()){
            *ck=map_[id];
            return true;
        }
        else
            return false;
    }

    void InsertChunks(const list<chunk>& ck, bool if_cnr){
        for(auto n:ck){
            n.Cnr_or_Recipe(if_cnr);
            if(LookUp(n)){
                map_[n.ID()] = n;
            }
            else{
                map_.emplace(n.ID(), n);
            }
        }
    }

    void EraseChunk(chunk ck){
        map_.erase(ck.ID());
    }

    void Load(const meta_data value);
    void Evict();
};
#endif //CONFIGURABLE_DEDUP_INDEX_TABLE_H
