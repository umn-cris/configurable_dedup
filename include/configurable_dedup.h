//
// Created by wyx on 19-6-20.
//

#ifndef CONFIGURABLE_DEDUP_CONFIGURABLE_DEDUP_H
#define CONFIGURABLE_DEDUP_CONFIGURABLE_DEDUP_H

#include "trace_reader.h"
#include "index_table.h"
#include "global.h"

class configurable_dedup{
private:
    hook_table hooks_;
    lru_cache cache_;
    sample_handler sampler_;
    long total_chunks_=0;
    long stored_chunks_=0;
    long sequence_number_=0;

public:

    void DoDedup();
    void CheckDW(int backup_version);
    void ReStore();
    bool Append2Containers(container* cnr);
    bool Append2Recipes(list<recipe>* segments);
    void CDSegmenting( vector<chunk>& window, recipe* re, list<recipe>* segments);
    bool IsBoundary(chunk ck, long num);
    void Load2cache(const list<chunk>& features);
    bool IfFeature(const chunk& ck){
        return sampler_.RandomPickFeature(ck,g_random_pick_ratio);
		/*if (!g_only_recipe) {
        	return sampler_.RandomPickFeature(ck,g_random_pick_ratio);
		} else {
        	if(sampler_.PositiveFeatures(ck,g_bit_num1)) return true;
        	if(sampler_.NegativeFeatures(ck,g_bit_num2)) return true;
		}*/

        return false;
    }
    long inline CnrScore(){
        return 1;
    }

    long inline RecipeScore(long feature_number){
        return feature_number;
    }

};
#endif //CONFIGURABLE_DEDUP_CONFIGURABLE_DEDUP_H
