//
// Created by wyx on 19-6-20.
//

#include "global.h"
bool g_dedup;
bool g_print_window_deduprate=false;
bool g_if_exact=false;
bool g_if_hybrid=false;
bool g_only_cnr=false;
bool g_only_recipe=false;
bool g_if_flush=false;
long g_cache_size=0;
long g_chunk_size=0;
long g_window_size=0;
long g_container_size=0;
long g_IO_cap=0;
long g_BFcache_size=0;
string g_dedup_trace_dir;
string g_trace_summary_file;
string g_recipe_file;
string g_sampling_method;
long g_bit_num1=0;
long g_bit_num2=0;
long g_bit_num3=0;
long g_bit_num4=0;
long g_random_pick_ratio=0;
long g_chunkID_pick_ratio=0;
long g_min_hook_number=0;
bool g_if_min_hook_sampling=false;
long IOloads = 0;
long cnr_IOloads=0;
long recipe_IOloads=0;
long g_segmenting_bit_num=0;
bool g_debug_output=true;
string g_dedup_engine;
long g_partition_number=0;
long g_recipe_version_bound = 10000;
long g_segment_size=1024;
string g_selection_policy="level";
bool g_uniform_for_cnr = false;
bool g_cap_adaptive = true;
double g_hybrid_recipe_ratio=0.8;

int Parse(string cfgfile){
    ifstream filestream(cfgfile, ios_base::in);
    if (filestream.fail()) {
        cerr << "open cfgfile:" << cfgfile << " fails!\n";
        return -1;
    }
    string line;

    while(getline(filestream, line)) {
        if (line.size()<=1 || line[0]== '#')
            continue;

        stringstream ss(line);
        string key, value;
        getline(ss, key, ' ');
        getline(ss, value, ' ');

        switch(hash_(key.c_str())){
            case hash_("cache_size"):
                g_cache_size = stol(value);
                break;
            case hash_("dedup_trace_dir"):
                g_dedup_trace_dir = value;
                break;
            case hash_("trace_summary_file"):
                g_trace_summary_file = value;
                break;
            case hash_("chunk_size"):
                g_chunk_size = stol(value);
                break;
            case hash_("window_size"):
                g_window_size = stol(value);
                break;
            case hash_("container_size"):
                g_container_size = stol(value);
                break;
            case hash_("IO_cap"):
                g_IO_cap = stol(value);
                break;
            case hash_("bit_num1"):
                g_bit_num1 = stol(value);
                break;
            case hash_("bit_num2"):
                g_bit_num2 = stol(value);
                break;
            case hash_("bit_num3"):
                g_bit_num3 = stol(value);
                break;
            case hash_("bit_num4"):
                g_bit_num4 = stol(value);
                break;
            case hash_("random_pick_ratio"):
                g_random_pick_ratio = stol(value);
                break;
            case hash_("chunkID_pick_ratio"):
                g_chunkID_pick_ratio = stol(value);
                break;
            case hash_("min_hook_number"):
                g_min_hook_number = stol(value);
                break;
            case hash_("segmenting_bit_num"):
                g_segmenting_bit_num = stol(value);
                break;
            case hash_("BFcache_size"):
                g_BFcache_size = stol(value);
                break;
            case hash_("only_cnr"):
                g_only_cnr = (value=="true");
                break;
            case hash_("if_min_hook_sampling"):
                g_if_min_hook_sampling = (value=="true");
                break;
            case hash_("only_recipe"):
                g_only_recipe = (value=="true");
                break;
            case hash_("if_exact"):
                g_if_exact = (value=="true");
                break;
            case hash_("if_hybrid"):
                    g_if_hybrid = (value=="true");
                    break;
            case hash_("debug_output"):
                    g_debug_output = (value=="true");
                    break;
            case hash_("recipe_version_bound"):
                g_recipe_version_bound = stol(value);
                break;
            case hash_("partition_number"):
                g_partition_number = stol(value);
                break;
            case hash_("segment_size"):
                g_segment_size = stol(value);
                break;
            case hash_("if_flush"):
                g_if_flush = (value=="true");
                break;
            case hash_("selection_policy"):
                g_selection_policy = value;
                break;
            case hash_("sampling_method"):
                g_sampling_method = value;
                break;
            case hash_("uniform_for_cnr"):
                g_uniform_for_cnr = (value=="true");
                break;
            case hash_("cap_adaptive"):
                g_cap_adaptive = (value == "true");
                break;
            case hash_("hybrid_recipe_ratio"):
                g_hybrid_recipe_ratio = stod(value);
                break;
            case hash_("print_window_deduprate"):
                g_print_window_deduprate = (value=="true");
                break;
            case hash_("dedup"):
                g_dedup = (value=="true");
                break;
            case hash_("recipe_file"):
                g_recipe_file = value;
                break;
            default:
                cout<<"unknown cfg: "<<key<<endl;
                return -1;
        }
    }
    return 0;
}
