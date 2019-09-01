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
		list<long> recipe_hook; //the ptr to the recipe uniform hooks only
		list<long> cnr_hook;
    list<long> recipe_ptr;  // for selection use
		list<long> cnr_ptr;    //for selection use

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

		long GetIndexSize() {
			return static_cast<long>(map_.size());
		}
		
		void InsertRecipeHook(const chunk &ck);
    void InsertRecipeFeature(const chunk &ck);
		void InsertCNRHook(const chunk &ck);
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
		long total_cnr_hit_;
		long cur_cnr_hit_;
		long cur_io_cap_;
		long t_win_num_;
		long cur_version_ = -1;

public:
    void DoDedup();
    bool Append2Containers(container* cnr);
    bool Append2Recipes(recipe* re);
    void CDSegmenting( vector<chunk>& window, recipe* re, list<recipe>* segments);
    bool IsBoundary(chunk ck,long num);
    void LoadSubset2cache(const list<meta_data> candidates);
		bool IfHook(const chunk& ck);
    bool IfFeature(const chunk& ck);
		list<meta_data> SelectSubsetDense(unordered_map<string, HookItem*> hook_map);
		list<meta_data> SelectSubsetNO(unordered_map<string, HookItem*> hook_map);
		list<meta_data> SelectSubset(unordered_map<string, HookItem*> hook_map);
		void AdjustIOCap(long can_num);
};


#endif //HYBRID_H
