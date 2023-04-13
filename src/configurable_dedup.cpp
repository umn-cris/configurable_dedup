//
// Created by wyx on 19-6-20.
//

#include <unordered_set>
#include "configurable_dedup.h"

vector<container> containers_;
vector<meta_data> recipes_meta;
int total_cnr_reads = 0;

struct ckComp{
    bool operator()(const chunk &a, const chunk &b){
        return a.ID()>b.ID(); // from big to small
    }
};
set<chunk,ckComp> min_hooks;

bool configurable_dedup::Append2Containers(container* cnr) {
    //cnr->SetSequenceNumber(sequence_number_);
    if (!g_only_recipe) {
        cnr->SetScore(CnrScore());
        cnr->IndicateCnr();
        containers_.push_back(*cnr);

        //sample hooks
        auto m = cnr->chunks_.begin();
        while(m!=cnr->chunks_.end()){
            if (IfFeature(*m)) {
                hooks_.InsertFeatures(*m, cnr->Meta());
            }
            m++;
        }
    }

    // reset will increase the meta.name by 1
    cnr->reset();
    return true;
}

bool configurable_dedup::Append2Recipes(recipe* n) {
    if (!g_only_cnr) {
        n->IndicateRecipe();
        recipes_meta.push_back(n->meta_);

        ofstream outfile;
        string recipe_path = g_recipe_path + to_string(n->Meta().Name());
        outfile.open(recipe_path);
        if (outfile.fail()) {
            cerr << "open "<< recipe_path <<  " failed!\n";
            exit(1);
        }


        auto c = n->chunks_.begin();
        while(c!=n->chunks_.end()){
            outfile << c->ID() << " " << c->GetLocation()<< " ";
            c++;
        }
        outfile.close();

        //sample hooks for the finished recipe
        auto m = n->chunks_.begin();
        while(m!=n->chunks_.end()){
            if (IfFeature(*m)) {
                hooks_.InsertFeatures(*m, n->Meta());
            }
            m++;
        }
    }

    // reset will increase the meta.name by 1
    n->reset();
    return true;
}

bool configurable_dedup::IsBoundary(chunk ck, long num) {
    if(g_segmenting_bit_num==0){
        if(num>0 && num%g_segment_size==0) return true;
    }else{
        if(sampler_.PositiveFeatures(ck,g_segmenting_bit_num)) return true;
    }
    return false;}

void configurable_dedup::Load2cache(const list<chunk>& features) {
    // we have multiple hook hit, each hook associates with several metagroups.
    // meta group selection problem: which hook's which meta group to pick

    //list<meta_data> candidates = hooks_.SimplePickCandidates(features);
    list<meta_data> candidates = hooks_.PickCandidates(features);

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
        if(IsBoundary(window[i],i)){
            re->SetSequenceNumber(sequence_number_);
            segments->push_back(*re);
            re->reset();
        }else{
            re->AppendChunk(window[i]);
        }
    }
    re->SetSequenceNumber(sequence_number_);
    segments->push_back(*re);
    re->reset();
}

bool compareFunction (subset a, subset b) {return a.Score()>b.Score();}

/*void configurable_dedup::CheckDW(int backup_version){
    container* current_cnr = new container(0,0,"container");

    // randomly select a DW to check.
    // for incremental gcc, DW can be small than 10
    unordered_set<string> all_chunks;



    string trace_sum, trace_name, trace_line;
    ifstream TraceSumFile;
    trace_sum = g_dedup_trace_dir + g_trace_summary_file;
    TraceSumFile.open(trace_sum);
    if (TraceSumFile.fail()) {
        cerr << "open "<< trace_sum <<  "failed!\n";
        exit(1);
    }

    int cur_backup_version = 0;
    while(getline(TraceSumFile, trace_line)) {
        stringstream ss(trace_line);
        getline(ss, trace_name, ' ');
        string trace_path;
        trace_path = g_dedup_trace_dir + trace_name;
        TraceReader *trace_ptr = new TraceReader(trace_path);

        cur_backup_version++;
        while (trace_ptr->HasNext()){

            long window_size=g_window_size;
            recipe* current_window = new recipe(0,0,"recipe");
            current_window->SetSequenceNumber(sequence_number_);

            //batch deduplication, each time grab a window size of chunks to do dedupe
            while(trace_ptr->HasNext() && window_size>0)
            {
                chunk ck = trace_ptr->Next();
                total_chunks_++;
                window_size--;
                current_window->AppendChunk(ck);
            }


            if (cur_backup_version==backup_version){
                unordered_set<string> cur_DW_CKs;
                for (auto c:current_window->chunks_)
                    cur_DW_CKs.insert(c.ID());

                //check all recipe, set the score as number of duplicates
                auto n=recipes_.begin();
                while (n!=recipes_.end()){
                    int hit=0;
                    for (auto nc:n->chunks_){
                        if (cur_DW_CKs.find(nc.ID())!=cur_DW_CKs.end())
                            hit++;
                    }
                    n->SetScore(hit);
                    n++;
                }

                //check all cnr, set the score as number of duplicates
                auto m=containers_.begin();
                while (m!=containers_.end()){
                    int hit=0;
                    for (auto mc:m->chunks_){
                        if (cur_DW_CKs.find(mc.ID())!=cur_DW_CKs.end())
                            hit++;
                    }
                    m->SetScore(hit);
                    m++;
                }

                //sort cnr and recipe based on score
                sort(recipes_.begin(),recipes_.end(),compareFunction);
                sort(containers_.begin(),containers_.end(),compareFunction);

                //calculate accumulated duplicates from recipe which has most duplicates to recipe which has 10th most duplicates
                auto recipe_it=recipes_.begin();
                int accumulated_duplicates=0;
                for (int i = 0; i < 10; ++i) {
                    // check how many duplicates the ith recipe can contribute
                    for (auto c:recipe_it->chunks_){
                        if (cur_DW_CKs.find(c.ID())!=cur_DW_CKs.end()){
                            accumulated_duplicates++;
                            // erase the duplicates, so later recipes will not repeatedly contribute
                            cur_DW_CKs.erase(cur_DW_CKs.find(c.ID()));
                        }
                    }
                    cout<<i<<"th recipe, accumulated duplicates:"<<accumulated_duplicates<<endl;
                    recipe_it++;
                }

                //calculate accumulated duplicates from cnr which has most duplicates to cnr which has 10th most duplicates
                auto cnr_it=containers_.begin();
                accumulated_duplicates=0;
                // reset cur_DW_CKs, cause it has erase some chunks when used to check recipe
                for (auto c:current_window->chunks_)
                    cur_DW_CKs.insert(c.ID());

                for (int i = 0; i < 10; ++i) {
                    // check how many duplicates the ith recipe can contribute
                    for (auto c:cnr_it->chunks_){
                        if (cur_DW_CKs.find(c.ID())!=cur_DW_CKs.end()){
                            accumulated_duplicates++;
                            // erase the duplicates, so later recipes will not repeatedly contribute
                            cur_DW_CKs.erase(cur_DW_CKs.find(c.ID()));
                        }
                    }
                    cout<<i<<"th cnr, accumulated duplicates:"<<accumulated_duplicates<<endl;
                    cnr_it++;
                }
                return;
            }


            //dedupe and store recipe & cnr
            auto m = current_window->chunks_.begin();
            while(m!=current_window->chunks_.end()) {
                auto it = all_chunks.find(m->ID());
                if(it==all_chunks.end()){
                    // new chunk, add to cnr. As for recipe, it will ingest this chunk later anyways
                    if(!current_cnr->AppendChunk(*m)){
                        Append2Containers(current_cnr);
                        current_cnr->AppendChunk(*m);
                    }
                }
                all_chunks.insert(m->ID());
                m++;
            }
            //Append2Containers(current_cnr);
            recipes_.push_back(*current_window);
            sequence_number_++;
        }
    }

}*/

void configurable_dedup::DoDedup(){
    container* current_cnr = new container(0,0,"container");
    recipe* current_recipe = new recipe(0,0,"recipe");
    //list<recipe>* segments_= new list<recipe>;
    long t_win_num_ = 0;
    long recipe_hook_num = 0;
    long cnr_hook_num = 0;
    long cache_hit=0, cache_miss=0, hook_hit=0;
    ofstream out_window_deduprate;

    string trace_sum, trace_name, trace_line;
    ifstream TraceSumFile;
    trace_sum = g_dedup_trace_dir + g_trace_summary_file;
    TraceSumFile.open(trace_sum);
    if (TraceSumFile.fail()) {
        cerr << "open "<< trace_sum <<  "failed!\n";
        exit(1);
    }


    while(getline(TraceSumFile, trace_line)) {

        long last_IOloads = IOloads;
        long last_cnr_IOloads = cnr_IOloads;
        long last_recipe_IOloads = recipe_IOloads;
        long last_total_chunks = total_chunks_;
        long last_stored_chunks = stored_chunks_;
        long cur_win = 0;


        stringstream ss(trace_line);
        getline(ss, trace_name, ' ');
        string trace_path;
        trace_path = g_dedup_trace_dir + trace_name;
        TraceReader trace_ptr(trace_path);
        if(g_print_window_deduprate){
            string outfile = trace_name+"window_deduprate";
            out_window_deduprate.open(outfile,ios::out);
        }



        while (trace_ptr.HasNext()){

            long window_size=g_window_size;
            dedupe_window window_;
            window_.SetSequenceNumber(++sequence_number_);


            long last_window_chunks = total_chunks_;
            long last_window_stored_chunks = stored_chunks_;

            //batch deduplication, each time grab a window size of chunks to do dedupe
            while(trace_ptr.HasNext() && window_size>0)
            {
                chunk ck = trace_ptr.Next();
                total_chunks_++;
                window_size--;
                window_.AppendChunk(ck);
            }

            cur_win++;
            t_win_num_++;
            //CDSegmenting(window_,current_recipe,segments_);
            list<chunk> hitted_hooks;

            //a batch of chunks will be first cut to multiple segments, each segment is a small set of chunks
            //segment is the basic unit to execute hook/sample/load cnr or recipe
            auto m = window_.chunks_.begin();
                /*1.hook hit*/
                while (m!=window_.chunks_.end()){
                    if(hooks_.LookUp(*m)){
                        hitted_hooks.push_back(*m);
                        hook_hit++;
                    }
                    m++;
                }

            /*2 according to recipe_features which hit the hook table, load champion subsets to lru_cache.
             * although only features belong to recipe that hit the hook table, those loaded subsets including both cnr and recipe
             * in here, old features(hit hook table) are recipe features while new features (not contained in hook table) are cnr features*/

            Load2cache(hitted_hooks);


            /*3. dedup via lru_cache*/
                // go over each chunk in the segment

            set<int> cnr_location;
            m = window_.chunks_.begin();
            while(m!=window_.chunks_.end()) {
                //cache hit
                   if(cache_.LookUp(*m)){
                        cache_hit++;
                       cnr_location.insert(m->GetLocation());
                   }
                   // or store unique chunk
                   else{
                        cache_miss++;
                        stored_chunks_++;
                       if(!current_cnr->AppendChunk(*m)){
                           Append2Containers(current_cnr);
                           current_cnr->AppendChunk(*m);
                           //if finish a container, will sample hooks in the container
                       }
                        // code for min sampling
                      /* if(g_if_min_hook_sampling){
                           if(min_hooks.size()<g_min_hook_number) {
                               min_hooks.insert(*m);
                           }
                           else{
                               string boundary = min_hooks.begin()->ID();
                               if( boundary > m->ID()) {
                                   min_hooks.erase(min_hooks.begin());
                                   min_hooks.insert(*m);
                               }
                           }
                       }*/
                   }
                   //load to current recipe, no matter it is deduped or not

                if (g_only_recipe){
                    if(!current_recipe->AppendChunk(*m)){
                        Append2Recipes(current_recipe);
                        current_recipe->AppendChunk(*m);
                    }
                }

                   m++;
            }
            total_cnr_reads += cnr_location.size();




            // every window pick a fix number of min chunk as hook
            /*if(g_if_min_hook_sampling){
                for(auto s:min_hooks){
                    containers_[s.GetLocation()].IndicateCnr();
                    hooks_.InsertFeatures(s, containers_[s.GetLocation()].Meta());
                    cnr_hook_num++;
                }
                min_hooks.clear();
            }*/

            if(g_if_flush) cache_.Flush();
            long current_window_chunks = total_chunks_ - last_window_chunks;
            long current_window_stored_chunks = stored_chunks_ - last_window_stored_chunks;
            double current_window_deduprate = current_window_chunks/(current_window_stored_chunks*1.0);
            if(g_print_window_deduprate){
                out_window_deduprate<<t_win_num_<<" "<<current_window_deduprate<<"\n";
            }
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
         long size=hooks_.cached_hooks_.size();
         //for(auto n:hooks_.map_)size += n.second.candidates_.size();
         long sample_ratio = total_chunks_/size;
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
	
        if (g_debug_output) {
        	cout<<"Sample_ratio"<<sample_ratio<<endl;
        	cout<<"hook_hit:"<<hook_hit<<" cache hit:"<<cache_hit<<" cache miss:"<<cache_miss<<endl;
        	cout<<"current cnr_IO:"<<current_cnr_IOloads<<" overall cnr_IO:"<<cnr_IOloads<<endl;
        	cout<<"current recipe_IO:"<<current_recipe_IOloads<<" overall recipe_IO:"<<recipe_IOloads<<endl;
        	cout<<"current IOloads:"<<current_IOloads<<" overall IOloads:"<<IOloads<<endl;
        	cout<<"current deduprate:"<<current_deduprate<<" overall deduprate:"<<overall_deduprate<<endl<<endl;
            if (g_only_recipe) cout<<"avg # cnr reads to restore a recipe:"<<total_cnr_reads/(t_win_num_*1.0)<<endl;
        } else {
            cout<<g_dedup_engine<<" "<<g_selection_policy<<" "<<g_cache_size<<" "<<g_container_size<<" "<<g_window_size<<" "<<g_IO_cap<<" "<<cur_win<<" "<<t_win_num_<<" "
                <<recipe_sample_ratio<<" "<<cnr_sample_ratio<<" "<<current_cnr_IOloads<<" "<<cnr_IOloads<<" "<<current_recipe_IOloads<<" "
                <<recipe_IOloads<<" "<<current_IOloads<<" "<<IOloads<<" "<<current_total_chunks<<" "<<total_chunks_<<" "
                <<current_stored_chunks<<" "<<stored_chunks_<<" "<<current_deduprate<<" "<<overall_deduprate<<"\n";
			}
        if(g_print_window_deduprate){
            out_window_deduprate.close();
        }
    }
    /*ofstream out_recipe("./recipe",ios::out);
    if(!out_recipe.is_open()) cout<<"open recipe faile"<<endl;*/
    /*vector<long> fragmentation;
    for(auto n:recipes_){
        unordered_set<long> set;
        for(auto m:n.chunks_){
            //cout<<m.GetLocation()<<"\n";
            set.emplace(m.GetLocation());
        }
        fragmentation.push_back(set.size());
    }
    //ofstream frag("fragmentation",ios::out);
    for(auto m:fragmentation){
        cout<<m<<endl;
    }*/
    //frag.close();
    //out_recipe.close();
}


