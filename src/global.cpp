//
// Created by wyx on 19-6-20.
//

#include "global.h"
bool g_if_exact=false;
bool g_if_hybrid=false;
bool g_only_cnr=false;
bool g_only_recipe=false;
long g_cache_size=0;
long g_chunk_size=0;
long g_window_size=0;
long g_container_size=0;
long g_IO_cap=0;
long g_BFcache_size=0;
string g_dedup_trace_dir;
string g_trace_summary_file;
long g_bit_num1=0;
long g_bit_num2=0;
long g_random_pick_ratio=0;
long IOloads = 0;
long cnr_IOloads=0;
long recipe_IOloads=0;
long g_segmenting_bit_num=0;
bool g_debug_output=true;
long g_dedup_engine_no=0;


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
            case hash_("random_pick_ratio"):
                g_random_pick_ratio = stol(value);
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
            default:
                cout<<"unknown cfg: "<<key<<endl;
                return -1;
        }
    }
    return 0;
}
