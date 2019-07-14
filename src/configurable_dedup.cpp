//
// Created by wyx on 19-6-20.
//

#include "configurable_dedup.h"

vector<container> containers_;
vector<recipe> recipes_;



bool configurable_dedup::Append2Containers(container* cnr) {
    cnr->SetSequenceNumber(sequence_number_);
    cnr->SetScore(CnrScore());
    containers_.push_back(*cnr);
    cnr->reset();
    return true;
}

bool configurable_dedup::Append2Recipes(recipe* re) {
    recipes_.push_back(*re);
    re->reset();
    return true;
}

bool configurable_dedup::IsBoundary(chunk ck) {
    return sampler_.PositiveFeatures(ck,g_segmenting_bit_num);
}

void configurable_dedup::Load2cache(const list<chunk>& features) {
    //list<meta_data> candidates = hooks_.PickCandidates(features);
    list<meta_data> candidates = hooks_.PickCandidatesFIFO(features);
    if(candidates.empty()) return;
    long cap=g_IO_cap;
    for(const auto n:candidates){
        if(cache_.Load(n))cap--;
        if(cap<=0)return;
    }
}

/* content depend segmenting method
         *  @input: a window of chunks
         *  @output: a set of segments
        */
void configurable_dedup::CDSegmenting( vector<chunk>& window, recipe* re,  list<recipe>* segments) {
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


void configurable_dedup::DoDedup(){

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


        stringstream ss(trace_line);
        getline(ss, trace_name, ' ');
        string trace_path;
        trace_path = g_dedup_trace_dir + trace_name;
        TraceReader *trace_ptr = new TraceReader(trace_path);
        while (trace_ptr->HasNext()){
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
            list<chunk> hitted_hooks;

            for(auto n:*segments_){
                list<chunk>::iterator m = n.chunks_.begin();
                    /*1. pick hook*/
                    while (m!=n.chunks_.end()){
                        if(hooks_.LookUp(*m)){
                            hitted_hooks.push_back(*m);
                            hook_hit++;
                        }
                        m++;
                    }
            }
            /*2 according to recipe_features which hit the hook table, load champion subsets to lru_cache.
             * although only features belong to recipe that hit the hook table, those loaded subsets including both cnr and recipe
             * in here, old features(hit hook table) are recipe features while new features (not contained in hook table) are cnr features*/

                Load2cache(hitted_hooks);


            /*3. dedup via lru_cache*/

            for(auto n:*segments_){
                list<chunk>::iterator m = n.chunks_.begin();
                while(m!=n.chunks_.end()){
                    //cout<<m->ID()<<endl;
                   if(IfFeature(*m) && !g_only_cnr) {
                         m->Cnr_or_Recipe(false);
                         recipe_features.push_back(*m);
                   }
                   if(cache_.LookUp(*m)){
                        cache_hit++;
                   }else{
                        cache_miss++;
                        stored_chunks_++;
                        if(!current_cnr->AppendChunk(*m)){
                            Append2Containers(current_cnr);
                            current_cnr->AppendChunk(*m);
                        }

						if(IfFeature(*m) && !g_only_recipe) {
								m->Cnr_or_Recipe(true);
								cnr_features.push_back(*m);
						}
                   }
                   m++;
                }
            }
            Append2Containers(current_cnr);


            /*4. update hooktable*/
            hooks_.InsertCnrFeatures(cnr_features);
            //hooks_.InsertRecipeFeatures(recipe_features);

        }
        //for(auto n:recipes_) cout<<n.Name()<<" "<<n.Score()<<" "<<n.SequenceNumber()<<endl;

//        hooks_.PrintHookInfo();
/*        {
            cout << "print cnr" << endl;
            for (auto m:containers_) cout << m.Name() << " " << m.SequenceNumber() << endl;
            cout << "print recipe" << endl;
            for (auto n:recipes_)cout << n.Name() << " " << n.SequenceNumber() << endl;
        }*/
         IOloads = cnr_IOloads+recipe_IOloads;
         long size=0;
         for(auto n:hooks_.map_)size += n.second.candidates_.size();
         long sample_ratio = total_chunks_/size;
        long current_cnr_IOloads = cnr_IOloads - last_cnr_IOloads;
        long current_recipe_IOloads = recipe_IOloads- last_recipe_IOloads;
        long current_IOloads = IOloads - last_IOloads;
        long current_total_chunks = total_chunks_ - last_total_chunks;
        long current_stored_chunks = stored_chunks_ - last_stored_chunks;
        double current_deduprate = current_total_chunks/(current_stored_chunks*1.0);
        double overall_deduprate = total_chunks_/(stored_chunks_*1.0);
        cout<<"Sample_ratio"<<sample_ratio<<endl;
        cout<<"hook_hit:"<<hook_hit<<" cache hit:"<<cache_hit<<" cache miss:"<<cache_miss<<endl;
        cout<<"current cnr_IO:"<<current_cnr_IOloads<<" overall cnr_IO:"<<cnr_IOloads<<endl;
        cout<<"current recipe_IO:"<<current_recipe_IOloads<<" overall recipe_IO:"<<recipe_IOloads<<endl;
        cout<<"current IOloads:"<<current_IOloads<<" overall IOloads:"<<IOloads<<endl;
        cout<<"current deduprate:"<<current_deduprate<<" overall deduprate:"<<overall_deduprate<<endl<<endl;
    }
}
