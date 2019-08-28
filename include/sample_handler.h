//
// Created by wyx on 19-6-20.
//
#include "global.h"
#ifndef DEDUP_REWRITE_SAMPLE_HANDLER_H
#define DEDUP_REWRITE_SAMPLE_HANDLER_H
class sample_handler{
public:
    long sample_gap_=0;
    bool PositiveFeatures(const chunk& ck, long bit_num);
    bool NegativeFeatures(const chunk& ck, long bit_num);
    bool RandomPickFeature(const chunk& ck, long sample_ratio);
    bool Segmenting(const chunk& ck, long bit_num);
};


#endif //DEDUP_REWRITE_SAMPLE_HANDLER_H
