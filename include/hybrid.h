//
// Created by Zhichao Cao on 19-7-30.
//

#ifndef HYBRID_H
#define HYBRID_H

#include "global.h"
#include "subset.h"
#include "sample_handler.h"
#include "index_table.h"

#include <utility>
#include <unordered_map>


class HookItem{
public:
    chunk ck_;
		list<meta_data> recipe_hook; //the ptr to the recipe uniform hooks only
    list<meta_data> recipe_ptr;  // for selection use
		list<meta_data> cnr_ptr;    //for selection use

};

class HookIndex: public index_table {
private:
public:
    unordered_map<string,HookItem> map_;

    bool LookUp(chunk ck){
        if(map_.find(ck.ID())!=map_.end())
            return true;
        else
            return false;
    }

    bool LookUp(string id, HookItem** entry){
        if(map_.find(id)!=map_.end()){
            *entry = &map_[id];
            return true;
        }
        else
            return false;
    }
		
		void InsertRecipeHook(const chunk &ck);
    void InsertRecipeFeature(const chunk &ck);
    void InsertCNRFeature(const chunk &ck);
		void EraseHookTable(chunk ck);
		void EraseChunk(chunk ck){
        EraseHookTable(ck);
    }
};



class HybridDedup{
private:
    HookIndex index_;
    lru_cache cache_;
    sample_handler sampler_;
    long total_chunks_=0;
    long stored_chunks_=0;
    long sequence_number_=0;

public:

    void DoDedup();
    bool Append2Containers(container* cnr);
    bool Append2Recipes(recipe* re);
    void CDSegmenting( vector<chunk>& window, recipe* re, list<recipe>* segments);
    bool IsBoundary(chunk ck);
    void LoadSubset2cache(const list<meta_data> candidates);
		bool IfRecipeHook(const chunk& ck);
    bool IfFeature(const chunk& ck);
		list<meta_data> SelectSubset(unordered_map<string, HookItem*> hook_map);
};


#endif //HYBRID_H
