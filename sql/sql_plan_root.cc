#include <iostream>
#include <vector> 
#include <list>

#include "sql/sql_plan_root.h"
#include "sql/join_optimizer/access_path.h"   // AccessPath
#include "sql/sql_lex.h"                      // Query_block
 


bool PLAN_ROOT::get_optimized_status(){
    return optimized_status;
};

void PLAN_ROOT::set_optimized_status(bool _status){
    optimized_status = _status;
};


/*
    Add new accesspath* to access_paths. Returns true if existing access_path has
    tha same query_block key as the incoming query_block - access_path pair. Otherwise, 
    returns false after successfully emplacing new access path. 
*/
bool PLAN_ROOT::add_access_path(Query_block* _query_block, AccessPath* _access_path){
    auto it = access_paths.emplace(_query_block, _access_path);
    if (!it.second) return true;
    return false;
};

void PLAN_ROOT::clear_access_paths(){
    access_paths.clear();
};

/*
    Does a lookup with key _query_block to check if an corresponding
    access path with exists within access_paths. 
bool PLAN_ROOT::access_path_exists(Query_block* _query_block){
    return access_paths.find(_query_block) != access_paths.end();
};
*/

/*
    Add a new set of params to params_sets. Eache set with params corresponds to 
    one execution of a prepared statment. If _clear == true, params_sets
    is cleared before insertion.  
*/
void PLAN_ROOT::set_param_set(std::vector<stmt_param> _param_set){
    param_set = _param_set;
};

std::vector<stmt_param> PLAN_ROOT::get_param_set(){
    return param_set;
}
    /*
    // Add param_set if first time stmt is being executed. 
    if (param_sets.size() == 0) {
        param_sets.push_back(_param_set);
        return;
    }
    */

    // Find similarity between existing set of parameters and new set. 
    //bool similar = compare_param_sets(param_sets.front(), _param_set);

    // If param_sets are not similar, throw the existing and ensure
    // regenaration of new access_path.
    /*
    if (!similar) {
        set_optimized_status(false);
        access_paths.clear();
        param_sets.clear();
        param_sets.push_back(_param_set);
    } 
    */

 
/*
    Estimates if the parmas between two statements. Should maybe be moved to plan_cache?
bool PLAN_ROOT::compare_param_sets(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2){
    
    // If number of parameters dont match the similarity is not sufficent.
    if (_s1.size() != _s2.size()) return false;
 
    // Compare each param of _s1 and _s2.   
    for (unsigned int i = 0; i < _s1.size(); i++) {
        if (_s1[i].param_type != _s2[i].param_type) return false;
        if (_s1[i].varname != _s2[i].varname) return false;
        if (_s1[i].val != _s2[i].val) return false;
    }
    return true;
};
*/