//
// Created by wyx on 19-6-20.
//
#include "global.h"
#ifndef DEDUP_REWRITE_SUBSET_H
#define DEDUP_REWRITE_SUBSET_H


class meta_data{
    long name_=0;
    bool if_cnr_= true;
    long score_=0;
    long sequence_number_=0;
public:
    void Set(long name, string location){
        name_ =name;
        if(location=="recipe")
            if_cnr_=false;
        if(location=="container")
            if_cnr_=true;
    }
    /*oid NumInc(){
        chunk_num_++;
    }*/
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
		/*long GetChunkNum() {
			return chunk_num_;
		}*/
};

class chunk{
    string chunk_id_;
    //long size_=0;
    long cnr_name_=-1;
    long recipe_name_=-1;
    bool if_cnr_=false;
public:
    chunk(){
        //size_=0;
        cnr_name_=-1;
        recipe_name_=-1;
        if_cnr_= false;
    }
    chunk(const chunk &value){
        chunk_id_=value.chunk_id_;
        //size_=value.size_;
        cnr_name_=value.cnr_name_;
        recipe_name_=value.recipe_name_;
        if_cnr_=value.if_cnr_;
    }
    ~chunk(){};
    void SetID(string id){
        chunk_id_=id;
    }
    void SetSize(long size){
        //size_=size;
    }
    void SetCnrName(long name){
        cnr_name_=name;
    }
    void SetRecipeName(long name){
        recipe_name_=name;
    }
    string ID()const{
        return chunk_id_;
    }
    bool IfCnr(){
        return if_cnr_;
    }
    void Cnr_or_Recipe(bool if_cnr){
        if_cnr_=if_cnr;
    }

    long CnrName()const{
        return cnr_name_;
    }

    long RecipeName()const{
        return recipe_name_;
    }
};

class subset{
private:
    meta_data meta_;
public:
    list<chunk> chunks_;
    subset(){}
    ~subset(){}
    subset(long chunk_num,long name, string location){
        meta_.Set(name, location);
    }

    bool AppendChunk(chunk& ck){
        if(!Meta().IfCnr()){
            ck.SetRecipeName(Name());
            chunks_.push_back(ck);
            //meta_.NumInc();
            return true;
        }else{
            if(chunks_.size()<g_container_size){
                ck.SetCnrName(Name());
                chunks_.push_back(ck);
                //meta_.NumInc();
                return true;
            }
            return false;
        }
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
		/*long GetChunkNum() {
			return meta_.GetChunkNum();
		}*/
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
};



#endif //DEDUP_REWRITE_SUBSET_H
