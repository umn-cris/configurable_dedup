//
// Created by wyx on 19-6-20.
//

#include <subset.h>
#include <unordered_map>
#include <utility>
#include "index_table.h"


extern vector<container> containers_;
extern vector<recipe> recipes_;

void hook_table::Leveling(list<pair<long, list<meta_data>>>& level_sort, list<meta_data>& recipe_cds_list){
    for (auto it:recipe_cds_list) {
        auto i=level_sort.begin();
        for (; i!=level_sort.end(); i++) {
            if (it.SequenceNumber() >= i->second.front().SequenceNumber()-3
                && it.SequenceNumber() <= i->second.front().SequenceNumber()+3) {
                i->second.push_back(it);
                i->first += it.Score();
                break;
            }
        }
        if (i == level_sort.end()) {
            list<meta_data> tmp_list;
            tmp_list.push_back(it);
            level_sort.push_back(make_pair(it.Score(), tmp_list));
        }
    }
}

list<meta_data> hook_table::PickCandidates(const list<chunk>& features) {
    list<meta_data> candidates;
    list<meta_data> tmp_candidates;
    unordered_map<long, long> recipe_cds;
    unordered_map<long, long> cnr_cds;

    for( auto n:features){
        tmp_candidates = LookUp(n.ID());
        for (auto it:tmp_candidates) {
          if (it.IfCnr()) {
            cnr_cds[it.Name()]++;
          } else {
            recipe_cds[it.Name()]++;
          }
        }    
    }
    list<meta_data> recipe_cds_list;
    list<meta_data> cnr_cds_list;
    for (auto it:recipe_cds) {
      recipe_cds_list.push_back(recipes_[it.first].Meta());
      recipe_cds_list.back().SetScore(it.second);
    }
    for (auto it:cnr_cds) {
      cnr_cds_list.push_back(containers_[it.first].Meta());
      cnr_cds_list.back().SetScore(it.second);
    }

	// begin of special leveling algorithm
	list<pair<long, list<meta_data>>> level_sort;
    Leveling(level_sort,recipe_cds_list);

    auto comp_level_sort = [](pair<long, list<meta_data>> s1,
                              pair<long, list<meta_data>> s2){
        return s1.first > s2.first;
    };

    level_sort.sort(comp_level_sort);

    if (!level_sort.empty()) {
        candidates.merge(level_sort.front().second);
    }
    candidates.merge(cnr_cds_list);
    //end of special leveling algorithm


    auto comp = [](meta_data s1, meta_data s2){
        return s1.Score()>s2.Score();
    };
    candidates.sort(comp);

/*    for(auto n:candidates)cout<<n.IfCnr()<<" "<<n.Score() << endl;
    cout<<endl;*/

    if(g_only_cnr){
        for(list<meta_data>::iterator it=candidates.begin();it!=candidates.end();){
            if(!it->IfCnr())  it = candidates.erase(it);
            else it++;
        }
    }
    if(g_only_recipe){
        for(list<meta_data>::iterator it=candidates.begin();it!=candidates.end();){
            if(it->IfCnr())  it = candidates.erase(it);
            else it++;
        }
    }

    //according to cap, pick the top k candidates
    if(candidates.size()>g_IO_cap){
        for (long i = candidates.size(); i > g_IO_cap; --i) {
            candidates.pop_back();
        }
    }

    return candidates;
}

list<meta_data> hook_table::PickCandidatesFIFO(const list<chunk>& features) {
    list<meta_data> candidates;
    list<meta_data> tmp_candidates;


    for( auto n:features){
        tmp_candidates = LookUp(n.ID());
        for (auto it:tmp_candidates) {
            candidates.push_back(it);
        }
    }

    if(g_only_cnr){
        for(list<meta_data>::iterator it=candidates.begin();it!=candidates.end();){
            if(!it->IfCnr())  it = candidates.erase(it);
            else it++;
        }
    }
    if(g_only_recipe){
        for(list<meta_data>::iterator it=candidates.begin();it!=candidates.end();){
            if(it->IfCnr())  it = candidates.erase(it);
            else it++;
        }
    }

    if(candidates.size()>g_IO_cap){
        for (long i = candidates.size(); i > g_IO_cap; --i) {
            candidates.pop_back();
        }
    }

    return candidates;
}

void hook_table::InsertRecipeFeatures(const list<chunk>& cks) {
    for(auto n:cks){
        hook_entry* entry;
        if (LookUp(n.ID(),&entry)) {
            entry->candidates_.push_back(recipes_[n.RecipeName()].Meta());
        } else {
            hook_entry tmp_entry;
            tmp_entry.ck_ = n;
            tmp_entry.candidates_.push_back(recipes_[n.RecipeName()].Meta());
            map_.emplace(n.ID(),tmp_entry);
        }
    }
}

void hook_table::InsertCnrFeatures(const list<chunk> &cks) {
    for(auto n:cks){
        hook_entry* entry;
        if (LookUp(n.ID(),&entry)) {
            entry->candidates_.push_back(containers_[n.CnrName()].Meta());
        } else {
            hook_entry tmp_entry;
            tmp_entry.ck_ = n;
            tmp_entry.candidates_.push_back(containers_[n.CnrName()].Meta());
            map_.emplace(n.ID(),tmp_entry);
        }
    }
}
void hook_table::EraseHookTable(chunk ck) {}



void lru_cache::Load(const meta_data value) {
    // if target subset is already loaded into cache, make it as the first subset in the lru list.
    /*if(subsets_.find(value.Name())!=subsets_.end() && subsets_[value.Name()]->IfCnr() == value.IfCnr()){
        lru_cache_.remove(value);
        lru_cache_.push_front(value);
        return;
    }*/
    for(auto n:lru_cache_){
        if(n.Name() == value.Name() && n.IfCnr() == value.IfCnr()) {
            lru_cache_.remove(value);
            lru_cache_.push_front(value);
            return;
        }
    }

    if (capacity_ <= lru_cache_.size())
    {
        Evict();
    }


    lru_cache_.push_front(value);
    if(value.IfCnr()){
        InsertChunks(containers_[value.Name()].chunks_, true);
        cnr_IOloads++;
    }else{
        InsertChunks(recipes_[value.Name()].chunks_, false);
        recipe_IOloads++;
    }
}

void lru_cache::Evict() {
    meta_data tmp = lru_cache_.back();
    if (tmp.IfCnr()) {
        for (auto m:containers_[tmp.Name()].chunks_) {
            EraseChunk(m);
        }
        lru_cache_.pop_back();
    }else{
        for(auto m:recipes_[tmp.Name()].chunks_){
            EraseChunk(m);
        }
        lru_cache_.pop_back();
    }
}
