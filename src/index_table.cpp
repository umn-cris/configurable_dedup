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

    if (!g_if_flush) {
        cout<<"Yixun! add the flush logic!"<<endl;
    }
    list<meta_data> candidates;
    list<meta_data> tmp_candidates;
    set<long> read_cans;
    bool jump_out = false;


    for( auto n:features){
        tmp_candidates = LookUp(n.ID());
        for (auto it:tmp_candidates) {
            if (g_only_cnr && it.IfCnr()) {
							read_cans.insert(it.Name());
							if(read_cans.size()>=g_IO_cap) {
								jump_out = true;
								break;
							}
						} else if (g_only_recipe && !it.IfCnr()) {
							read_cans.insert(it.Name());
							if(read_cans.size()>=g_IO_cap) {
								jump_out = true;
								break;
							}							
						}
        }
				if (jump_out) {
					break;
				}
    }

		for (auto it:read_cans) {
			if (g_only_cnr) {
				candidates.push_back(containers_[it].Meta());
			} else if (g_only_recipe) {
				candidates.push_back(recipes_meta[it]);
			}
		}

/*

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

*/
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
				selected_subsets.push_back(recipes_meta[cds_selected[i]]);
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

list<meta_data> hook_table::SimplePickCandidates(const list<chunk>& features) {
    list<meta_data> temp_candidates;
    list<meta_data> candidates;

    for(auto n:features){
        //every feature is possible to associate with meta groups
        temp_candidates = LookUp(n.ID());
        candidates.merge(temp_candidates);
    }
    candidates.unique();
    return candidates;
}

list<meta_data> hook_table::PickCandidates(const list<chunk>& features) {
    list<meta_data> pick_candidates;
    list<meta_data> hook_associate_subsets;
    unordered_map<long, long> recipe_candidates_hit_num;
    unordered_map<long, long> cnr_candidates_hit_num;
    unordered_map<long, set<string>> recipe_candidates_map;
    unordered_map<long, set<string>> cnr_candidates_map;

    //record the occurrence frequency of hooks' associated subsets
    for(auto n:features){
        hook_associate_subsets = LookUp(n.ID());
        /*for (auto s:hook_associate_subsets) {
            cout<<s.Name()<<" ";
        }*/
        for (auto it:hook_associate_subsets) {
            if (it.IfCnr()) {
                if (cnr_candidates_hit_num.find(it.Name())!=cnr_candidates_hit_num.end())
                    cnr_candidates_hit_num[it.Name()]++;
                else
                    cnr_candidates_hit_num.emplace(it.Name(),1);

                if (cnr_candidates_map.find(it.Name())!=cnr_candidates_map.end())
                    cnr_candidates_map[it.Name()].insert(n.ID());
                else{
                    set<string> s;
                    s.insert(n.ID());
                    cnr_candidates_map.emplace(it.Name(),s);
                }
            } else {
                if (recipe_candidates_hit_num.find(it.Name())!=recipe_candidates_hit_num.end())
                    recipe_candidates_hit_num[it.Name()]++;
                else
                    recipe_candidates_hit_num.emplace(it.Name(),1);

                if (recipe_candidates_map.find(it.Name())!=recipe_candidates_map.end())
                    recipe_candidates_map[it.Name()].insert(n.ID());
                else{
                    set<string> s;
                    s.insert(n.ID());
                    recipe_candidates_map.emplace(it.Name(),s);
                }
            }
        }    
    }

    //
    list<meta_data> recipe_cds_list;
    list<meta_data> cnr_cds_list;

    if (g_only_cnr) {
	    for (auto it:cnr_candidates_hit_num) {
      	    cnr_cds_list.push_back(containers_[it.first].Meta());
      	    cnr_cds_list.back().SetScore(it.second);
    	}
			if (g_selection_policy == "fifo") {
				pick_candidates = SelectFIFO(features);
			} else if (g_selection_policy == "level") {
                pick_candidates = SelectLevel(cnr_cds_list);
			} else if (g_selection_policy == "sparse") {
                pick_candidates = SelectSparse(cnr_candidates_map);
			} else if (g_selection_policy == "sort") {
                pick_candidates = SelectSort(cnr_cds_list);
			}

    } else if (g_only_recipe) {
    	for (auto it:recipe_candidates_hit_num) {
      	    recipe_cds_list.push_back(recipes_meta[it.first]);
      	    recipe_cds_list.back().SetScore(it.second);
    	}
			if (g_selection_policy == "fifo") {
                pick_candidates = SelectFIFO(features);
			} else if (g_selection_policy == "level") {
                pick_candidates = SelectLevel(recipe_cds_list);
			} else if (g_selection_policy == "sparse") {
                pick_candidates = SelectSparse(recipe_candidates_map);
			} else if (g_selection_policy == "sort") {
                pick_candidates = SelectSort(recipe_cds_list);
			}
		}
    //thinking about what subset I want to load as motivation experiments with I/O cap
    // rank
    return pick_candidates;
}


void hook_table::InsertFeatures(const chunk &cks, meta_data meta) {
        hook_entry* entry;
        if (!LookUp(cks.ID(),&entry)) {
            hook_entry tmp_entry;
            tmp_entry.candidates_.push_back(meta);
            cached_hooks_.emplace(cks.ID(),tmp_entry);
        } else {
            // check whether this feature has already get association with the subset
            bool exist=false;
            for(auto c:entry->candidates_){
                if (c.Name() == meta.Name()) exist= true;
            }
            // only if not associated already, push back the subset
            if (!exist)
                entry->candidates_.push_back(meta);
        }
}

void hook_table::EraseHookTable(chunk ck) {}



bool lru_cache::Load(const meta_data value) {
    // if target subset is already loaded into cache, make it as the first subset in the lru list.
    /*for(auto n:lru_cache_){
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
*/

    lru_cache_.push_front(value);
    if(value.IfCnr()){
        InsertChunks(containers_[value.Name()].chunks_, true);
        cnr_IOloads++;
    }else{
        list<chunk> cks;

        ifstream infile;
        string recipe_name = g_recipe_path + to_string(value.Name());
        infile.open(recipe_name);
        if (infile.fail()) {
            cerr << "open "<< recipe_name <<  "failed!\n";
            exit(1);
        }
        string s;
        while( infile >> s )
        {
            chunk ck;
            ck.SetID(s);
            infile >> s;
            ck.SetLocation(stol(s));
            cks.push_back(ck);
        }

        infile.close();



        InsertChunks(cks, false);

        recipe_IOloads++;
    }
    return true;

}

/*void lru_cache::Evict() {
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
}*/

void lru_cache::Flush() {
    cached_chunks_.clear();
    lru_cache_.clear();
}
