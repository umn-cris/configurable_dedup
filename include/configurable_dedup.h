//
// Created by wyx on 19-6-20.
//

#ifndef CONFIGURABLE_DEDUP_CONFIGURABLE_DEDUP_H
#define CONFIGURABLE_DEDUP_CONFIGURABLE_DEDUP_H

#include "trace_reader.h"
#include "index_table.h"
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
    bool Append2Containers(container* cnr);
    bool Append2Recipes(recipe* re);
    void CDSegmenting( vector<chunk>& window, recipe* re, list<recipe>* segments);
    bool IsBoundary(chunk ck);
    void Load2cache(const list<chunk>& features);
    bool IfFeature(const chunk& ck){
		/*
        if(1){
            return sampler_.RandomPickFeature(ck,g_random_pick_ratio);
        }
		*/
        if(sampler_.PositiveFeatures(ck,g_bit_num1)) return true;
        if(sampler_.NegativeFeatures(ck,g_bit_num2)) return true;

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
