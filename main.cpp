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
					HybridDedup dedup;
					dedup.DoDedup();
				} else {
        	configurable_dedup dedup_process;
        	dedup_process.DoDedup();
				}
		}
		else{
        bloom_store bloombloom;
        bloombloom.DoDedup();
    }
    return 0;
}
