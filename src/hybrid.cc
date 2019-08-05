//
// Created by Zhichao Cao, 2019/07/30.
//

#include "hybrid.h"
#include "trace_reader.h"

void HookIndex::InsertRecipeHook(const chunk &ck) {
	HookItem* entry;
  if (LookUp(ck.ID(),&entry)) {
  	entry->recipe_hook.push_back(recipes_[ck.RecipeName()].Meta());
  } else {
    HookItem tmp_entry;
    tmp_entry.ck_ = ck;
    tmp_entry.recipe_hook.push_back(recipes_[ck.RecipeName()].Meta());
    map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertRecipeFeature(const chunk &ck) {
	HookItem* entry;
  if (LookUp(ck.ID(),&entry)) {
  	entry->recipe_ptr.push_back(recipes_[ck.RecipeName()].Meta());
  } else {
    HookItem tmp_entry;
    tmp_entry.ck_ = ck;
    tmp_entry.recipe_ptr.push_back(recipes_[ck.RecipeName()].Meta());
    map_.emplace(ck.ID(),tmp_entry);
  }
}

void HookIndex::InsertCNRFeature(const chunk &ck) {
	HookItem* entry;
  if (LookUp(ck.ID(),&entry)) {
     entry->cnr_ptr.push_back(containers_[ck.CnrName()].Meta());
  } else {
     HookItem tmp_entry;
     tmp_entry.ck_ = ck;
     tmp_entry.cnr_ptr.push_back(containers_[ck.CnrName()].Meta());
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

bool HybridDedup::IfRecipeHook(const chunk& ck) {
	if(sampler_.PositiveFeatures(ck,g_bit_num1)) return true;
	if(sampler_.NegativeFeatures(ck,g_bit_num2)) return true;
	return false;
}


bool HybridDedup::IfFeature(const chunk& ck) {
	return sampler_.RandomPickFeature(ck,g_random_pick_ratio);
	return false;
}


list<meta_data> HybridDedup::SelectSubset(list<HookItem*> hook_entries) {
	list<meta_data> selected_subsets;
	
	return selected_subsets; 
}


void HybridDedup::DoDedup() {

    container* current_cnr = new container(0,0,"container");
    recipe* current_recipe = new recipe(0,0,"recipe");
    list<recipe>* segments_= new list<recipe>;

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


            segments_->clear();
            CDSegmenting(window_,current_recipe,segments_);
            list<chunk> recipe_features;
            list<chunk> cnr_features;
            list<HookItem*> hitted_hook_entries;

            for(auto n:*segments_){
              list<chunk>::iterator m = n.chunks_.begin();
              /*1. pick hook*/
              while (m!=n.chunks_.end()){
								HookItem* tmp_hook_ptr;
              	if(index_.LookUp((*m).ID(), &tmp_hook_ptr)){
                	hitted_hook_entries.push_back(tmp_hook_ptr);
                	hook_hit++;
              	}
              	m++;
            	}
            }
            /*2 according to recipe_features which hit the hook table, load champion subsets to lru_cache.
             * although only features belong to recipe that hit the hook table, those loaded subsets including both cnr and recipe
             * in here, old features(hit hook table) are recipe features while new features (not contained in hook table) are cnr features*/

            LoadSubset2cache(SelectSubset(hitted_hook_entries));


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
                }
								if(IfRecipeHook(*m)) {
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
        cout<<"hook_hit:"<<hook_hit<<" cache hit:"<<cache_hit<<" cache miss:"<<cache_miss<<endl;
        cout<<"current cnr_IO:"<<current_cnr_IOloads<<" overall cnr_IO:"<<cnr_IOloads<<endl;
        cout<<"current recipe_IO:"<<current_recipe_IOloads<<" overall recipe_IO:"<<recipe_IOloads<<endl;
        cout<<"current IOloads:"<<current_IOloads<<" overall IOloads:"<<IOloads<<endl;
        cout<<"current deduprate:"<<current_deduprate<<" overall deduprate:"<<overall_deduprate<<endl<<endl;
    }
}
