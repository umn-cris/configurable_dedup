//
// Created by wyx on 19-6-20.
//

#include <subset.h>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <utility>
#include "index_table.h"

list<meta_data> hook_table::SelectFIFO(const list<chunk>& features) {
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

    return candidates;
}


list<meta_data> hook_table::SelectLevel(list<meta_data> recipe_cds_list){
	list<meta_data> candidates;
	list<pair<long, list<meta_data>>> level_sort;
	long level_size = 2*g_window_size / g_container_size;
    for (auto it:recipe_cds_list) {
        auto i=level_sort.begin();
        for (; i!=level_sort.end(); i++) {
            if (it.SequenceNumber() >= i->second.front().SequenceNumber()-level_size
                && it.SequenceNumber() <= i->second.front().SequenceNumber()+level_size) {
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

		auto comp_level_sort = [](pair<long, list<meta_data>> s1,
                              pair<long, list<meta_data>> s2){
      return s1.first > s2.first;
    };

    level_sort.sort(comp_level_sort);
    if (!level_sort.empty()) {
        candidates = level_sort.front().second;
    }
		return candidates;
}


list<meta_data> hook_table::SelectSparse(unordered_map<long, set<string>> cds_map) {
	list<meta_data> selected_subsets;
	list<meta_data> tmp_candidates;
	long k = cds_map.size();
	vector<long> cds_selected(k, -1);
	list<meta_data> cds_cans;
	set<string> selected_hooks;

	for(long i=0; i<k; i++) {
		long s = 0;
		set<string> top_set;
		for (auto it = cds_map.begin(); it!=cds_map.end(); it++) {
			if(it->second.size() > s) {
				s = it->second.size();
				top_set = it->second;
				cds_selected[i] = it->first;
			}
		}

		for (auto it = top_set.begin(); it!=top_set.end(); it++) {
			tmp_candidates = LookUp(*it);
			for (auto l = tmp_candidates.begin(); l != tmp_candidates.end(); l++) {
				if (g_only_cnr && l->IfCnr()) {
					cds_map[l->Name()].erase(*it);
				} else if (g_only_recipe) {
					cds_map[l->Name()].erase(*it);
				}
			}
		}

		if (cds_map.find(cds_selected[i]) != cds_map.end()) {
			if (g_only_cnr) {
				selected_subsets.push_back(containers_[cds_selected[i]].Meta());
			} else if (g_only_recipe) {
				selected_subsets.push_back(recipes_[cds_selected[i]].Meta());
			}
			cds_map.erase(cds_selected[i]);
			selected_hooks.insert(top_set.begin(), top_set.end());
			//cout<<"recipe_selected: "<<recipe_selected[i]<<" "<<top_set.size()<<endl;
		}	
	}
	return selected_subsets;
}


list<meta_data> hook_table::SelectSort(list<meta_data> cds_list) {
  auto comp = [](meta_data s1, meta_data s2){
    return s1.Score()>s2.Score();
  };

	cds_list.sort(comp);
	return cds_list;
}


list<meta_data> hook_table::PickCandidates(const list<chunk>& features) {
    list<meta_data> candidates;
    list<meta_data> tmp_candidates;
    unordered_map<long, long> recipe_cds;
    unordered_map<long, long> cnr_cds;
		unordered_map<long, set<string>> recipe_map;
		unordered_map<long, set<string>> cnr_map;

    for( auto n:features){
        tmp_candidates = LookUp(n.ID());
        for (auto it:tmp_candidates) {
          if (it.IfCnr()) {
            cnr_cds[it.Name()]++;
						cnr_map[it.Name()].insert(n.ID());
          } else {
            recipe_cds[it.Name()]++;
						recipe_map[it.Name()].insert(n.ID());
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

		if (g_only_cnr) {
			if (g_selection_policy == "fifo") {
				candidates = SelectFIFO(features);
			} else if (g_selection_policy == "level") {
				candidates = SelectLevel(cnr_cds_list);
			} else if (g_selection_policy == "sparse") {
				candidates = SelectSparse(cnr_map);
			} else if (g_selection_policy == "sort") {
				candidates = SelectSort(cnr_cds_list);
			}

		} else if (g_only_recipe) {
			if (g_selection_policy == "fifo") {
				candidates = SelectFIFO(features);
			} else if (g_selection_policy == "level") {
				candidates = SelectLevel(recipe_cds_list);
			} else if (g_selection_policy == "sparse") {
				candidates = SelectSparse(recipe_map);
			} else if (g_selection_policy == "sort") {
				candidates = SelectSort(recipe_cds_list);
			}
		}

    return candidates;
}


void hook_table::InsertRecipeFeatures(const chunk &cks, long recipe_name) {
        hook_entry* entry;
        if (LookUp(cks.ID(),&entry)) {
            entry->candidates_.push_back(recipes_[recipe_name].Meta());
        } else {
            hook_entry tmp_entry;
            tmp_entry.candidates_.push_back(recipes_[recipe_name].Meta());
            map_.emplace(cks.ID(),tmp_entry);
        }
}

void hook_table::InsertCnrFeatures(const chunk &cks, meta_data meta) {
        hook_entry* entry;
        if (LookUp(cks.ID(),&entry)) {
            entry->candidates_.push_back(meta);
        } else {
            hook_entry tmp_entry;
            tmp_entry.candidates_.push_back(meta);
            map_.emplace(cks.ID(),tmp_entry);
        }
}
void hook_table::EraseHookTable(chunk ck) {}



bool lru_cache::Load(const meta_data value) {
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
            return false;
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
    return true;

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

void lru_cache::Flush() {
    map_.clear();
    lru_cache_.clear();
}
