//
// Created by Zhichao Cao, 2019/07/30.
//

#include "hybrid.h"
#include "trace_reader.h"
#include <set>
#include <algorithm>
#include <cmath>
int iii=1;

void HookIndex::InsertRecipeHook(const chunk &ck, long recipe_name) {
	HookItem* entry;
	//cout<<"add recipe hook: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
  	entry->recipe_hook.push_back(recipe_name);
  } else {
    HookItem tmp_entry;
    tmp_entry.recipe_hook.push_back(recipe_name);
    map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertRecipeFeature(const chunk &ck, long recipe_name) {
	HookItem* entry;
	//cout<<"add recipe feature: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
  	entry->recipe_ptr.push_back(recipe_name);
  } else {
    HookItem tmp_entry;
    tmp_entry.recipe_ptr.push_back(recipe_name);
    map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertCNRHook(const chunk &ck, long cnr_name) {
	HookItem* entry;
	//cout<<"add cnr feature: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
     entry->cnr_hook.push_back(cnr_name);
  } else {
     HookItem tmp_entry;
     tmp_entry.cnr_hook.push_back(cnr_name);
     map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertCNRFeature(const chunk &ck, long cnr_name) {
	HookItem* entry;
	//cout<<"add cnr feature: "<<ck.ID()<<endl;
  if (LookUp(ck.ID(),&entry)) {
     entry->cnr_ptr.push_back(cnr_name);
  } else {
     HookItem tmp_entry;
     tmp_entry.cnr_ptr.push_back(cnr_name);
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

bool HybridDedup::IsBoundary(chunk ck, long num) {
    //if(sampler_.Segmenting(ck,g_segmenting_bit_num)) return true;
    if(g_segmenting_bit_num==0){
        if(num>0 && num%g_segment_size==0) return true;
    }else{
        if(sampler_.PositiveFeatures(ck,g_segmenting_bit_num)) return true;
    }
    return false;
}

void HybridDedup::LoadSubset2cache(const list<meta_data> candidates) {
    if(candidates.empty()) {
			return;
		}
		long io=0;
    for(const auto n:candidates) {
        if(cache_.Load(n)) {
					io++;
				}
        if(io>=cur_io_cap_) {
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
        if(IsBoundary(window[i],i)){
            re->SetSequenceNumber(sequence_number_);
            segments->push_back(*re);
            Append2Recipes(re);
        }else{
            re->AppendChunk(window[i]);
        }
    }
    segments->push_back(*re);
		re->SetScore(cur_version_);
    Append2Recipes(re);
}

bool HybridDedup::IfHook(const chunk& ck) {
	if(sampler_.PositiveFeatures(ck,g_bit_num1)){
	    cout<<iii++<<endl;
        return true;
    }
	if(sampler_.NegativeFeatures(ck,g_bit_num2)){
        cout<<iii++<<endl;
	    return true;
    }
	return false;
}


bool HybridDedup::IfFeature(const chunk& ck) {
	return sampler_.RandomPickFeature(ck,g_random_pick_ratio);
	return false;
}


/*The dense based design$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/
list<meta_data> HybridDedup::SelectSubsetDense(unordered_map<string, HookItem*> hook_map) {
	list<meta_data> selected_subsets;
	/*
	unordered_map<long, set<string>> recipe_map;  //the map from recipe to its hitted hook chunk id
	unordered_map<long, set<string>> recipe_ptr_map;
	unordered_map<long, set<string>> cnr_ptr_map;

	for (auto it = hook_map.begin(); it!=hook_map.end(); it++) {
		string id = it->first;
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

	cur_cnr_hit_ = cnr_ptr_map.size();
	total_cnr_hit_ += cur_cnr_hit_;

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
  */
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
		string id = it->first;
		for (auto i=it->second->recipe_hook.begin(); i!=it->second->recipe_hook.end(); i++) {
			if (cur_version_ - g_recipe_version_bound <= recipes_[*i].Score()) {
				recipe_map[*i].insert(id);
			}
		}
		for (auto i=it->second->recipe_ptr.begin(); i!=it->second->recipe_ptr.end(); i++) {
			if (cur_version_ - g_recipe_version_bound <= recipes_[*i].Score()) {
				recipe_ptr_map[*i].insert(id);
			}
		}
		for (auto i=it->second->cnr_hook.begin(); i!=it->second->cnr_hook.end(); i++) {
			cnr_map[*i].insert(id);
		}
		for (auto i=it->second->cnr_ptr.begin(); i!=it->second->cnr_ptr.end(); i++) {
			cnr_ptr_map[*i].insert(id);
		}
	}

	cur_cnr_hit_ = cnr_ptr_map.size();
	total_cnr_hit_ += cur_cnr_hit_;

	/*second, select the recipe candidates*/
	long bound = g_window_size / g_container_size;
	long k=bound;
	if (g_IO_cap > bound) {
		k = max(static_cast<int>(bound/2), static_cast<int>(k*2 - g_IO_cap));
	}
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
		string id = it->first;
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

	cur_cnr_hit_ = cnr_ptr_map.size();
	total_cnr_hit_ += cur_cnr_hit_;

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


void HybridDedup::AdjustIOCap(long can_num) {
	if (can_num <= g_IO_cap) {
		cur_io_cap_ = g_IO_cap;
		return;
	}
	long cur_io = cnr_IOloads+recipe_IOloads;
	long exp_io = t_win_num_ * g_IO_cap;
	long credit = exp_io - cur_io;

	double ave_hit = static_cast<double>(total_cnr_hit_) / static_cast<double>(t_win_num_);
	if (static_cast<double>(cur_cnr_hit_) < ave_hit) {
		cur_io_cap_ = g_IO_cap;
		return;
	}
	long cal_io = static_cast<long>(ceil(g_IO_cap * static_cast<double>(cur_cnr_hit_)/ave_hit));
	cur_io_cap_ = min(static_cast<int>(g_cache_size), min(static_cast<int>(credit), static_cast<int>(cal_io)));
	cur_io_cap_ = min(static_cast<int>(g_cache_size), static_cast<int>(credit));
	return;
}


void HybridDedup::DoDedup() {

    container* current_cnr = new container(0,0,"container");
    recipe* current_recipe = new recipe(0,0,"recipe");
    list<recipe>* segments_= new list<recipe>;
		vector<long> version_recipe_hook_num;
		t_win_num_ = 0;
		long recipe_hook_num = 0;
		long last_recipe_hook_num = 0;
		long cnr_hook_num = 0;

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
				long cur_win = 0;


        stringstream ss(trace_line);
				cur_version_++;
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
						t_win_num_++;
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

			cur_io_cap_ = g_IO_cap;
			list<meta_data> selected_subsets = SelectSubsetNO(hitted_hook_map);
			AdjustIOCap(static_cast<long>(selected_subsets.size()));
			//cout<<selected_subsets.size()<<" "<<cur_io_cap_<<endl;
            LoadSubset2cache(selected_subsets);


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
										index_.InsertRecipeFeature(*m, n.Name());
										index_.InsertCNRFeature(*m, current_cnr->Name());
										cnr_hook_num++;
									}
									if(IfHook(*m)) {
                		m->Cnr_or_Recipe(true);
                  	index_.InsertCNRHook(*m, current_cnr->Name());
										cnr_hook_num++;
                	}
                }
								if(IfHook(*m)) {
                	m->Cnr_or_Recipe(false);
                  index_.InsertRecipeHook(*m, n.Name());
									recipe_hook_num++;
                }
                m++;
              }
            }
            if(g_if_flush) cache_.Flush();
        }

        IOloads = cnr_IOloads+recipe_IOloads;
				double recipe_sample_ratio, cnr_sample_ratio;
				if (recipe_hook_num == 0) {
					recipe_sample_ratio = 0.0;
				} else {
        	recipe_sample_ratio = total_chunks_/(recipe_hook_num * 1.0);
				}
				if (cnr_hook_num == 0) {
					cnr_sample_ratio = 0.0;
				} else {
        	cnr_sample_ratio = stored_chunks_/(cnr_hook_num * 1.0);
				}
        long current_cnr_IOloads = cnr_IOloads - last_cnr_IOloads;
        long current_recipe_IOloads = recipe_IOloads- last_recipe_IOloads;
        long current_IOloads = IOloads - last_IOloads;
        long current_total_chunks = total_chunks_ - last_total_chunks;
        long current_stored_chunks = stored_chunks_ - last_stored_chunks;
        double current_deduprate = current_total_chunks/(current_stored_chunks*1.0);
        double overall_deduprate = total_chunks_/(stored_chunks_*1.0);

				version_recipe_hook_num.push_back(recipe_hook_num-last_recipe_hook_num);
				last_recipe_hook_num = recipe_hook_num;
				long stored_recipe_hook_num=0, version_count = g_recipe_version_bound;
				for(long i = version_recipe_hook_num.size() - 1; i>=0; i--) {
					stored_recipe_hook_num +=version_recipe_hook_num[i];
					version_count--;
					if(version_count<0) {
						break;
					}
				}

				if (g_debug_output) {
        	cout<<"recipe_sample_ratio: "<<recipe_sample_ratio<<endl;
					cout<<"cnr_sample_ratio: "<<cnr_sample_ratio<<endl;
					cout<<"total windows: "<<t_win_num_<<" current windows: "<<cur_win<<endl;
					cout<<"total FP: "<<index_.GetIndexSize()<<" total pointers: "<<recipe_hook_num+cnr_hook_num<<" stored recipe hooks: "<<stored_recipe_hook_num<<endl;
        	cout<<"hook_hit:"<<hook_hit<<" cache hit:"<<cache_hit<<" cache miss:"<<cache_miss<<endl;
        	cout<<"current cnr_IO:"<<current_cnr_IOloads<<" overall cnr_IO:"<<cnr_IOloads<<endl;
        	cout<<"current recipe_IO:"<<current_recipe_IOloads<<" overall recipe_IO:"<<recipe_IOloads<<endl;
        	cout<<"current IOloads:"<<current_IOloads<<" overall IOloads:"<<IOloads<<endl;
        	cout<<"current deduprate:"<<current_deduprate<<" overall deduprate:"<<overall_deduprate<<endl<<endl;
			} else{
				// output the regular results:
				cout<<g_dedup_engine_no<<" "<<g_cache_size<<" "<<g_container_size<<" "<<g_window_size<<" "<<g_IO_cap<<" "<<cur_win<<" "<<t_win_num_<<" "
						<<index_.GetIndexSize()<<" "<<stored_recipe_hook_num<<" "<<cnr_hook_num<<" "<<stored_recipe_hook_num+cnr_hook_num<<" "
						<<recipe_sample_ratio<<" "<<cnr_sample_ratio<<" "<<current_cnr_IOloads<<" "<<cnr_IOloads<<" "<<current_recipe_IOloads<<" "
						<<recipe_IOloads<<" "<<current_IOloads<<" "<<IOloads<<" "<<current_total_chunks<<" "<<total_chunks_<<" "
						<<current_stored_chunks<<" "<<stored_chunks_<<" "<<current_deduprate<<" "<<overall_deduprate<<"\n";
			}
    }
}
