//
// Created by Zhichao Cao, 2019/07/30.
//

#include "hybrid.h"
#include "trace_reader.h"
#include <set>
#include <algorithm>
#include <cmath>

void HookIndex::InsertRecipeHook(const chunk &ck) {
	HookItem* entry;
	//cout<<"add recipe hook: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
  	entry->recipe_hook.push_back(ck.RecipeName());
  } else {
    HookItem tmp_entry;
    tmp_entry.ck_ = ck;
    tmp_entry.recipe_hook.push_back(ck.RecipeName());
    map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertRecipeFeature(const chunk &ck) {
	HookItem* entry;
	//cout<<"add recipe feature: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
  	entry->recipe_ptr.push_back(ck.RecipeName());
  } else {
    HookItem tmp_entry;
    tmp_entry.ck_ = ck;
    tmp_entry.recipe_ptr.push_back(ck.RecipeName());
    map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertCNRHook(const chunk &ck) {
	HookItem* entry;
	//cout<<"add cnr feature: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
     entry->cnr_hook.push_back(ck.CnrName());
  } else {
     HookItem tmp_entry;
     tmp_entry.ck_ = ck;
     tmp_entry.cnr_hook.push_back(ck.CnrName());
     map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertCNRFeature(const chunk &ck) {
	HookItem* entry;
	//cout<<"add cnr feature: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
     entry->cnr_ptr.push_back(ck.CnrName());
  } else {
     HookItem tmp_entry;
     tmp_entry.ck_ = ck;
     tmp_entry.cnr_ptr.push_back(ck.CnrName());
     map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::EraseHookTable(chunk ck) {}

bool HybridDedup::Append2Containers(container* cnr) {
    cnr->SetSequenceNumber(sequence_number_);
    cnr->SetScore(0);
    containers_.push_back(*cnr);
    cnr->reset();
    return true;
}

bool HybridDedup::Append2Recipes(recipe* re) {
    recipes_.push_back(*re);
    re->reset();
    return true;
}

bool HybridDedup::IsBoundary(chunk ck) {
    return sampler_.PositiveFeatures(ck,g_segmenting_bit_num);
}

void HybridDedup::LoadSubset2cache(const list<meta_data> candidates) {
    if(candidates.empty()) {
			return;
		}
    long cap=g_IO_cap;
    for(const auto n:candidates) {
        if(cache_.Load(n))cap--;
        if(cap<=0) {
					return;
				}
    }
}

/* content depend segmenting method
         *  @input: a window of chunks
         *  @output: a set of segments
        */
void HybridDedup::CDSegmenting( vector<chunk>& window, recipe* re,  list<recipe>* segments) {
    for (int i = 0; i < window.size(); ++i) {
        if(IsBoundary(window[i])){
            re->SetSequenceNumber(sequence_number_);
            segments->push_back(*re);
            Append2Recipes(re);
        }else{
            re->AppendChunk(window[i]);
        }
    }
    segments->push_back(*re);
    Append2Recipes(re);
}

bool HybridDedup::IfHook(const chunk& ck) {
	if(sampler_.PositiveFeatures(ck,g_bit_num1)) return true;
	if(sampler_.NegativeFeatures(ck,g_bit_num2)) return true;
	return false;
}


bool HybridDedup::IfFeature(const chunk& ck) {
	return sampler_.RandomPickFeature(ck,g_random_pick_ratio);
	return false;
}


/*The dense based design$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
list<meta_data> HybridDedup::SelectSubsetDense(unordered_map<string, HookItem*> hook_map) {
	list<meta_data> selected_subsets;
	
	unordered_map<long, set<string>> recipe_map;  //the map from recipe to its hitted hook chunk id
	unordered_map<long, set<string>> recipe_ptr_map;
	unordered_map<long, set<string>> cnr_ptr_map;

	/*first, insert the mapping of recipe->hookset*/
	for (auto it = hook_map.begin(); it!=hook_map.end(); it++) {
		string id = it->second->ck_.ID();
		for (auto i=it->second->recipe_hook.begin(); i!=it->second->recipe_hook.end(); i++) {
			recipe_map[*i].insert(id);
		}
		for (auto i=it->second->recipe_ptr.begin(); i!=it->second->recipe_ptr.end(); i++) {
			recipe_ptr_map[*i].insert(id);
		}
		for (auto i=it->second->cnr_ptr.begin(); i!=it->second->cnr_ptr.end(); i++) {
			cnr_ptr_map[*i].insert(id);
		}
	}

	/*second, select the recipe candidates*/
	long k=static_cast<long>(recipe_map.size());
	vector<long> recipe_selected(k, -1);
	list<meta_data> recipe_cans;

	for(long i=0; i<k; i++) {
		double s = -1;
		set<string> top_set;
		for (auto it = recipe_map.begin(); it!=recipe_map.end(); it++) {
			double dense = it->second.size() / static_cast<double>(recipes_[it->first].Meta().GetChunkNum());
			if(dense > s) {
				s = dense;
				top_set = it->second;
				recipe_selected[i] = it->first;
			}
		}

		for (auto it = top_set.begin(); it!=top_set.end(); it++) {
			for (auto l = hook_map[*it]->recipe_hook.begin(); l != hook_map[*it]->recipe_hook.end(); l++) {
				recipe_map[*l].erase(*it);
			}
		}

		if (recipe_map.find(recipe_selected[i]) != recipe_map.end()) {
			recipe_map.erase(recipe_selected[i]);
			cout<<"recipe_selected: "<<recipe_selected[i]<<" "<<s<<endl;
		}

		
	}

	/*Third, exclude the hooks in the selected recipe segments*/
	long num = min(5, static_cast<int>(k));
	for (long i=0; i < num; i++) {
		if (recipe_selected[i] != -1) {
			selected_subsets.push_back(recipes_[recipe_selected[i]].Meta());
			
			if (recipe_ptr_map.find(recipe_selected[i]) != recipe_ptr_map.end()) {
				for (auto it = recipe_ptr_map[recipe_selected[i]].begin(); it!= recipe_ptr_map[recipe_selected[i]].end(); it++) {
					for (auto l=hook_map[*it]->cnr_ptr.begin(); l!= hook_map[*it]->cnr_ptr.end(); l++) {
						cnr_ptr_map[*l].erase(*it);
					}
				}
			}
			
		}
	}


	/*forth, select the cnr candidates*/
	long h=static_cast<long>(cnr_ptr_map.size());
	vector<long> cnr_selected(h, -1);
	list<meta_data> cnr_cans;	

	for(long i=0; i<h; i++) {
		double s = 0;
		set<string> top_set;
		for (auto it = cnr_ptr_map.begin(); it!=cnr_ptr_map.end(); it++) {
			double dense = it->second.size() / static_cast<double>(containers_[it->first].Meta().GetChunkNum());
			if(dense > s) {
				s = dense;
				top_set = it->second;
				cnr_selected[i] = it->first;
			}
		}

		for (auto it = top_set.begin(); it!=top_set.end(); it++) {
			for (auto l = hook_map[*it]->cnr_ptr.begin(); l != hook_map[*it]->cnr_ptr.end(); l++) {
				cnr_ptr_map[*l].erase(*it);
			}
		}
		if (cnr_selected[i] > 0 && cnr_selected[i] < containers_.size()) {
			cout<<"cnr_selected: "<<cnr_selected[i]<<" "<<s<<endl;
			selected_subsets.push_back(containers_[cnr_selected[i]].Meta());
		}

		if (cnr_ptr_map.find(cnr_selected[i]) != cnr_ptr_map.end()) {
			cnr_ptr_map.erase(cnr_selected[i]);
		}
	}
    
	return selected_subsets; 
}


/*The no overlap design##########################################################################################*/
list<meta_data> HybridDedup::SelectSubsetNO(unordered_map<string, HookItem*> hook_map) {
	list<meta_data> selected_subsets;
	
	unordered_map<long, set<string>> recipe_map;  //the map from recipe to its hitted hook chunk id
	unordered_map<long, set<string>> cnr_map;
	unordered_map<long, set<string>> recipe_ptr_map;
	unordered_map<long, set<string>> cnr_ptr_map;

	/*first, insert the mapping of recipe->hookset*/
	for (auto it = hook_map.begin(); it!=hook_map.end(); it++) {
		string id = it->second->ck_.ID();
		for (auto i=it->second->recipe_hook.begin(); i!=it->second->recipe_hook.end(); i++) {
			recipe_map[*i].insert(id);
		}
		for (auto i=it->second->recipe_ptr.begin(); i!=it->second->recipe_ptr.end(); i++) {
			recipe_ptr_map[*i].insert(id);
		}
		for (auto i=it->second->cnr_hook.begin(); i!=it->second->cnr_hook.end(); i++) {
			cnr_map[*i].insert(id);
		}
		for (auto i=it->second->cnr_ptr.begin(); i!=it->second->cnr_ptr.end(); i++) {
			cnr_ptr_map[*i].insert(id);
		}
	}

	/*second, select the recipe candidates*/
	long k=6;
	vector<long> recipe_selected(k, -1);
	list<meta_data> recipe_cans;
	set<string> selected_hooks;

	for(long i=0; i<k; i++) {
		long s = 0;
		set<string> top_set;
		for (auto it = recipe_map.begin(); it!=recipe_map.end(); it++) {
			if(it->second.size() > s) {
				s = it->second.size();
				top_set = it->second;
				recipe_selected[i] = it->first;
			}
		}

		for (auto it = top_set.begin(); it!=top_set.end(); it++) {
			for (auto l = hook_map[*it]->recipe_hook.begin(); l != hook_map[*it]->recipe_hook.end(); l++) {
				recipe_map[*l].erase(*it);
			}
		}

		if (recipe_map.find(recipe_selected[i]) != recipe_map.end()) {
			selected_subsets.push_back(recipes_[recipe_selected[i]].Meta());
			recipe_map.erase(recipe_selected[i]);
			selected_hooks.insert(top_set.begin(), top_set.end());
			//cout<<"recipe_selected: "<<recipe_selected[i]<<" "<<top_set.size()<<endl;
		}

		
	}

	/*Third, exclude the cnrs that have high overlap with the data in the selected set*/
	for (auto it = cnr_map.begin(); it !=cnr_map.end(); it++) {
			double ratio = 0.8;
			long bound = floor(ratio*it->second.size());
			long found = 0;
			for(auto i=it->second.begin(); i!=it->second.end(); i++) {
				if(selected_hooks.find(*i) != selected_hooks.end()) {
					found++;
				}
			}
			if (found > bound && cnr_ptr_map.find(it->first) != cnr_ptr_map.end()) {
				cnr_ptr_map.erase(it->first);
			}
	}


	/*forth, select the cnr candidates*/
	long h=static_cast<long>(cnr_ptr_map.size());
	vector<long> cnr_selected(h, -1);
	list<meta_data> cnr_cans;	

	for(long i=0; i<h; i++) {
		long s = 0;
		set<string> top_set;
		for (auto it = cnr_ptr_map.begin(); it!=cnr_ptr_map.end(); it++) {
			if(it->second.size() > s) {
				s = it->second.size();
				top_set = it->second;
				cnr_selected[i] = it->first;
			}
		}
		if (cnr_selected[i] > 0 && cnr_selected[i] < containers_.size()) {
			//cout<<"cnr_selected: "<<cnr_selected[i]<<" "<<top_set.size()<<endl;
			selected_subsets.push_back(containers_[cnr_selected[i]].Meta());
		}

		if (cnr_ptr_map.find(cnr_selected[i]) != cnr_ptr_map.end()) {
			cnr_ptr_map.erase(cnr_selected[i]);
		}
	}
	return selected_subsets; 
}


/*The original design******************************************************************************8*/
list<meta_data> HybridDedup::SelectSubset(unordered_map<string, HookItem*> hook_map) {
	list<meta_data> selected_subsets;
	
	unordered_map<long, set<string>> recipe_map;  //the map from recipe to its hitted hook chunk id
	unordered_map<long, set<string>> recipe_ptr_map;
	unordered_map<long, set<string>> cnr_ptr_map;

	/*first, insert the mapping of recipe->hookset*/
	for (auto it = hook_map.begin(); it!=hook_map.end(); it++) {
		string id = it->second->ck_.ID();
		for (auto i=it->second->recipe_hook.begin(); i!=it->second->recipe_hook.end(); i++) {
			recipe_map[*i].insert(id);
		}
		for (auto i=it->second->recipe_ptr.begin(); i!=it->second->recipe_ptr.end(); i++) {
			recipe_ptr_map[*i].insert(id);
		}
		for (auto i=it->second->cnr_ptr.begin(); i!=it->second->cnr_ptr.end(); i++) {
			cnr_ptr_map[*i].insert(id);
		}
	}

	/*second, select the recipe candidates*/
	long k=6;
	vector<long> recipe_selected(k, -1);
	list<meta_data> recipe_cans;

	for(long i=0; i<k; i++) {
		long s = 0;
		set<string> top_set;
		for (auto it = recipe_map.begin(); it!=recipe_map.end(); it++) {
			if(it->second.size() > s) {
				s = it->second.size();
				top_set = it->second;
				recipe_selected[i] = it->first;
			}
		}

		for (auto it = top_set.begin(); it!=top_set.end(); it++) {
			for (auto l = hook_map[*it]->recipe_hook.begin(); l != hook_map[*it]->recipe_hook.end(); l++) {
				recipe_map[*l].erase(*it);
			}
		}

		if (recipe_map.find(recipe_selected[i]) != recipe_map.end()) {
			recipe_map.erase(recipe_selected[i]);
			//cout<<"recipe_selected: "<<recipe_selected[i]<<" "<<top_set.size()<<endl;
		}

		
	}

	/*Third, exclude the hooks in the selected recipe segments*/
	for (long i=0; i < k; i++) {
		if (recipe_selected[i] != -1) {
			selected_subsets.push_back(recipes_[recipe_selected[i]].Meta());
			/*
			if (recipe_ptr_map.find(recipe_selected[i]) != recipe_ptr_map.end()) {
				for (auto it = recipe_ptr_map[recipe_selected[i]].begin(); it!= recipe_ptr_map[recipe_selected[i]].end(); it++) {
					for (auto l=hook_map[*it]->cnr_ptr.begin(); l!= hook_map[*it]->cnr_ptr.end(); l++) {
						cnr_ptr_map[*l].erase(*it);
					}
				}
			}
			*/
		}
	}


	/*forth, select the cnr candidates*/
	long h=static_cast<long>(cnr_ptr_map.size());
	vector<long> cnr_selected(h, -1);
	list<meta_data> cnr_cans;	

	for(long i=0; i<h; i++) {
		long s = 0;
		set<string> top_set;
		for (auto it = cnr_ptr_map.begin(); it!=cnr_ptr_map.end(); it++) {
			if(it->second.size() > s) {
				s = it->second.size();
				top_set = it->second;
				cnr_selected[i] = it->first;
			}
		}
		/*
		for (auto it = top_set.begin(); it!=top_set.end(); it++) {
			for (auto l = hook_map[*it]->cnr_ptr.begin(); l != hook_map[*it]->cnr_ptr.end(); l++) {
				cnr_ptr_map[*l].erase(*it);
			}
		}
		*/
		if (cnr_selected[i] > 0 && cnr_selected[i] < containers_.size()) {
			//cout<<"cnr_selected: "<<cnr_selected[i]<<" "<<top_set.size()<<endl;
			selected_subsets.push_back(containers_[cnr_selected[i]].Meta());
		}

		if (cnr_ptr_map.find(cnr_selected[i]) != cnr_ptr_map.end()) {
			cnr_ptr_map.erase(cnr_selected[i]);
		}
	}
	return selected_subsets; 
}


void HybridDedup::DoDedup() {

    container* current_cnr = new container(0,0,"container");
    recipe* current_recipe = new recipe(0,0,"recipe");
    list<recipe>* segments_= new list<recipe>;
		long total_win = 0;

    string trace_sum, trace_name, trace_line;
    ifstream TraceSumFile;
    trace_sum = g_dedup_trace_dir + g_trace_summary_file;
    TraceSumFile.open(trace_sum);
    if (TraceSumFile.fail()) {
        cerr << "open "<< trace_sum <<  "failed!\n";
        exit(1);
    }

    while(getline(TraceSumFile, trace_line)) {
        long cache_hit=0, cache_miss=0, hook_hit=0;
        long last_IOloads = IOloads;
        long last_cnr_IOloads = cnr_IOloads;
        long last_recipe_IOloads = recipe_IOloads;
        long last_total_chunks = total_chunks_;
        long last_stored_chunks = stored_chunks_;
				long recipe_hook_num = 0;
				long cnr_hook_num = 0;
				long cur_win = 0;


        stringstream ss(trace_line);
        getline(ss, trace_name, ' ');
        string trace_path;
        trace_path = g_dedup_trace_dir + trace_name;
        TraceReader *trace_ptr = new TraceReader(trace_path);
        while (trace_ptr->HasNext()) {
            long window_size=g_window_size;
            vector<chunk> window_;
            sequence_number_++;

            while(trace_ptr->HasNext() && window_size>0)
            {
                chunk ck = trace_ptr->Next();
                total_chunks_++;
                window_size--;
                window_.push_back(ck);
            }

						cur_win++;
						total_win++;
            segments_->clear();
            CDSegmenting(window_,current_recipe,segments_);
            list<chunk> recipe_features;
            list<chunk> cnr_features;
            unordered_map<string, HookItem*> hitted_hook_map;

            for(auto n:*segments_){
              list<chunk>::iterator m = n.chunks_.begin();
              /*1. pick hook*/
              while (m!=n.chunks_.end()){
								HookItem* tmp_hook_ptr;
              	if(index_.LookUp((*m).ID(), &tmp_hook_ptr) && hitted_hook_map.find((*m).ID()) == hitted_hook_map.end()){
                	hitted_hook_map[(*m).ID()] = tmp_hook_ptr;
                	hook_hit++;
              	}
              	m++;
            	}
            }
            /*2 according to recipe_features which hit the hook table, load champion subsets to lru_cache.
             * although only features belong to recipe that hit the hook table, those loaded subsets including both cnr and recipe
             * in here, old features(hit hook table) are recipe features while new features (not contained in hook table) are cnr features*/

            LoadSubset2cache(SelectSubsetNO(hitted_hook_map));


            /*3. dedup via lru_cache and process the hook selection*/

            for(auto n:*segments_){
            	list<chunk>::iterator m = n.chunks_.begin();
              while(m!=n.chunks_.end()){
                //cout<<m->ID()<<endl;
                if(cache_.LookUp(*m)){
                	cache_hit++;
                } else {
                  cache_miss++;
                  stored_chunks_++;
                  if(!current_cnr->AppendChunk(*m)){
                     Append2Containers(current_cnr);
                     current_cnr->AppendChunk(*m);
                  }
									if(IfFeature(*m) && !g_only_recipe) {
										m->Cnr_or_Recipe(true);
										index_.InsertRecipeFeature(*m);
										index_.InsertCNRFeature(*m);
										cnr_hook_num++;
									}
									if(IfHook(*m)) {
                		m->Cnr_or_Recipe(true);
                  	index_.InsertCNRHook(*m);
                	}
                }
								if(IfHook(*m)) {
                	m->Cnr_or_Recipe(false);
                  index_.InsertRecipeHook(*m);
									recipe_hook_num++;
                }
                m++;
              }
            }
				}

        IOloads = cnr_IOloads+recipe_IOloads;
        long recipe_sample_ratio = total_chunks_/recipe_hook_num;
				long cnr_sample_ratio = stored_chunks_/cnr_hook_num;
        long current_cnr_IOloads = cnr_IOloads - last_cnr_IOloads;
        long current_recipe_IOloads = recipe_IOloads- last_recipe_IOloads;
        long current_IOloads = IOloads - last_IOloads;
        long current_total_chunks = total_chunks_ - last_total_chunks;
        long current_stored_chunks = stored_chunks_ - last_stored_chunks;
        double current_deduprate = current_total_chunks/(current_stored_chunks*1.0);
        double overall_deduprate = total_chunks_/(stored_chunks_*1.0);
        cout<<"recipe_sample_ratio: "<<recipe_sample_ratio<<endl;
				cout<<"cnr_sample_ratio: "<<cnr_sample_ratio<<endl;
				cout<<"total windows: "<<total_win<<" current windows: "<<cur_win<<endl;
        cout<<"hook_hit:"<<hook_hit<<" cache hit:"<<cache_hit<<" cache miss:"<<cache_miss<<endl;
        cout<<"current cnr_IO:"<<current_cnr_IOloads<<" overall cnr_IO:"<<cnr_IOloads<<endl;
        cout<<"current recipe_IO:"<<current_recipe_IOloads<<" overall recipe_IO:"<<recipe_IOloads<<endl;
        cout<<"current IOloads:"<<current_IOloads<<" overall IOloads:"<<IOloads<<endl;
        cout<<"current deduprate:"<<current_deduprate<<" overall deduprate:"<<overall_deduprate<<endl<<endl;
    }
}
