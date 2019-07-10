//
// Created by wyx on 19-6-20.
//

#include <subset.h>
#include "index_table.h"


extern vector<container> containers_;
extern vector<recipe> recipes_;

list<meta_data> hook_table::PickCandidates(const list<chunk>& features) {
    list<meta_data> candidates;
    list<meta_data> tmp_candidates;

    for( auto n:features){
        tmp_candidates = LookUp(n.ID());
        candidates.merge(tmp_candidates);
    }

    auto comp = [](meta_data s1, meta_data s2){
        return s1.Score()>s2.Score();
    };
    candidates.sort(); // sort candidates according to their name
    candidates.unique(); // remove deduplicate candidates
    candidates.sort(comp); // sort candidates according to their score

    for(auto n:candidates)cout<<n.IfCnr()<<" "<<n.Score() << endl;
cout<<endl;
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


void hook_table::InsertRecipeFeatures(const list<chunk>& cks) {
    for(auto n:cks){
        hook_entry* entry;
        LookUp(n.ID(),&entry);
        entry->candidates_.push_back(recipes_[n.RecipeName()].Meta());
    }
}

void hook_table::InsertCnrFeatures(const list<chunk> &cks) {
    for(auto n:cks){
        hook_entry entry;
        entry.ck_=n;
        entry.candidates_.push_back(containers_[n.CnrName()].Meta());
        map_.emplace(n.ID(),entry);
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