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
					g_dedup_engine_no = 2;
					HybridDedup dedup;
					dedup.DoDedup();
				} else {
					if (g_only_cnr && !g_only_recipe) {
						g_dedup_engine_no = 3;
					} else {
						g_dedup_engine_no = 4;
					}
        	configurable_dedup dedup_process;
        	dedup_process.DoDedup();
				}
		}
		else{
				g_dedup_engine_no = 1;
        bloom_store bloombloom;
        bloombloom.DoDedup();
    }
    return 0;
}
