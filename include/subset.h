//
// Created by wyx on 19-6-20.
//
#include "global.h"
#ifndef DEDUP_REWRITE_SUBSET_H
#define DEDUP_REWRITE_SUBSET_H


class meta_data{
    long chunk_num_=0;
    long name_=0;
    bool if_cnr_= true;
    long score_=0;
    long sequence_number_=0;
public:
    void Set(long chunk_num,long name, string location){
        chunk_num_=chunk_num;
        name_ =name;
        if(location=="recipe")
            if_cnr_=false;
        if(location=="container")
            if_cnr_=true;
    }
    void NumInc(){
        chunk_num_++;
    }
    void NameInc(){
        name_++;
    }
    long Name()const{
        return name_;
    }
    long Score()const{
        return score_;
    }
    long SequenceNumber()const{
        return sequence_number_;
    }
    void SetSequenceNumber(long number){
        sequence_number_=number;
    }
    void SetScore(long score){
        score_=score;
    }
    bool IfCnr()const{
        return if_cnr_;
    }
    void IndicateCnr(){
        if_cnr_=true;
    }

    void IndicateRecipe(){
        if_cnr_=false;
    }

    bool operator ==(meta_data d2){
        return (this->Name()==d2.Name() && this->IfCnr() == d2.IfCnr());
    }
    bool operator <(meta_data d2){
        return (this->Name() < d2.Name());
    }
    long GetChunkNum() {
        return chunk_num_;
    }
};

class chunk{
    string chunk_id_;
    long location_;
    //long cnr_name_=-1;
    //long recipe_name_=-1;
    bool if_cnr_=false;
public:
    chunk(){
        location_=0;
        //cnr_name_=-1;
        //recipe_name_=-1;
        if_cnr_= false;
    }
    chunk(const chunk &value){
        chunk_id_=value.chunk_id_;
        location_=value.location_;
        //cnr_name_=value.cnr_name_;
        //recipe_name_=value.recipe_name_;
        if_cnr_=value.if_cnr_;
    }
    ~chunk(){};
    void SetID(string id){
        chunk_id_=id;
    }

    void SetLocation(long position){
        location_=position;
    }
/*
    void SetCnrName(long name){
        cnr_name_=name;
    }
    void SetRecipeName(long name){
        recipe_name_=name;
    }
*/
    string ID()const{
        return chunk_id_;
    }
    long GetLocation(){
        return location_;
    }
    bool IfCnr(){
        return if_cnr_;
    }
    void Cnr_or_Recipe(bool if_cnr){
        if_cnr_=if_cnr;
    }
/*
    long CnrName()const{
        return cnr_name_;
    }

    long RecipeName()const{
        return recipe_name_;
    }
*/
};

class subset{

public:
    meta_data meta_;
    list<chunk> chunks_;
    subset(){}
    ~subset(){}
    subset(long chunk_num,long name, string location){
        meta_.Set(chunk_num, name, location);
    }

    long Name(){
        return meta_.Name();
    }

    long Score()const{
        return meta_.Score();
    }

    long SequenceNumber(){
        return meta_.SequenceNumber();
    }

    void SetSequenceNumber(long number){
        meta_.SetSequenceNumber(number);
    }
    void SetScore(long score){
        meta_.SetScore(score);
    }
    meta_data Meta()const{
        return meta_;
    }

    void reset(){
        chunks_.clear();
        meta_.NameInc();
    }
    void IndicateCnr(){
        meta_.IndicateCnr();
    }

    void IndicateRecipe(){
        meta_.IndicateRecipe();
    }
    long GetChunkNum() {
        return meta_.GetChunkNum();
    }
};

class container: public subset{
public:
    container(){
        IndicateCnr();
    }
    container(long chunk_num,long name, string location){
        IndicateCnr();
        subset(chunk_num, name, location);
    }
    ~container(){}

    bool AppendChunk(chunk& ck){
        // recipe container share same size
        if(chunks_.size()<g_container_size){
            //ck.SetCnrName(Name());
            ck.SetLocation(meta_.Name());
            chunks_.push_back(ck);
            meta_.NumInc();
            return true;
        } else{
            return false;
        }
    }
};

class recipe: public subset{
public:
    recipe(){
        IndicateRecipe();
    }
    recipe(long chunk_num,long name, string location){
        IndicateRecipe();
        subset( chunk_num, name, location);
    }
    ~recipe(){}

    bool AppendChunk(chunk& ck){
        // recipe & container share same size
        if(chunks_.size()<g_container_size){
            chunks_.push_back(ck);
            meta_.NumInc();
            return true;
        } else{
            return false;
        }
    }
};

class dedupe_window: public subset{
public:
    dedupe_window(){
            IndicateRecipe();
    }
    dedupe_window(long chunk_num,long name, string location){
        IndicateRecipe();
        subset(chunk_num, name, location);
    }
    ~dedupe_window(){}

    bool AppendChunk(chunk& ck){
        // dedupe window may have size different from cnr
        if(chunks_.size()<g_window_size){
            //ck.SetCnrName(Name());
            chunks_.push_back(ck);
            meta_.NumInc();
            return true;
        } else{
            return false;
        }
    }
};


#endif //DEDUP_REWRITE_SUBSET_H
