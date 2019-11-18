#include "global.h"
#include "configurable_dedup.h"
#include "bloom_store.h"
#include "hybrid.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr<<"argc must be 2"<<endl;
        return -1;
    }

    string cfgfile = argv[1];

    if (Parse(cfgfile)) {
        cerr<< "parse config file " << cfgfile << " failed!\n";
        return -1;
    }
    if(!g_if_exact)
    {
        if (g_if_hybrid) {
            g_dedup_engine = "hybrid";
            HybridDedup dedup;
            dedup.DoDedup();
        } else {
            if (g_only_cnr && !g_only_recipe) {
                g_dedup_engine = "cnr only";
            } else {
                g_dedup_engine = "recipe only";
            }
        configurable_dedup dedup_process;
        dedup_process.DoDedup();
        }
    }
    else{
        g_dedup_engine = "bloomStore";
        bloom_store bloombloom;
        bloombloom.DoDedup();
    }
    return 0;
}
