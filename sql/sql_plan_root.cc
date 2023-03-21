#include <iostream>
#include <vector> 
#include <list>

#include "sql/sql_plan_root.h"
#include "sql/join_optimizer/access_path.h"   // AccessPath
#include "sql/sql_lex.h"                      // Query_block
 
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

/*
    Does a lookup with key _query_block to check if an corresponding
    access path with exists within access_paths. 
*/
bool PLAN_ROOT::access_path_exists(Query_block* _query_block){
    return access_paths.find(_query_block) != access_paths.end();
};

/*
    Store a new param in vector<stmt_param> params. 
void PLAN_ROOT::add_param(stmt_param _stmt_param){
    params.push_back(_stmt_param);
};
*/

/*
    Clear all params stored in vector<stmt_param> params. 
void PLAN_ROOT::clear_params(){
    params.clear();
};
*/

/*
    Add a new set of params to params_sets. Eache set with params corresponds to 
    one execution of a prepared statment. If _clear == true, params_sets
    is cleared before insertion.  
*/
void PLAN_ROOT::add_param_set(std::vector<stmt_param> _param_set){
    
    // Add param_set if first time stmt is being executed. 
    if (param_sets.size() == 0) {
        param_sets.push_back(_param_set);
        return;
    }

    // Find similarity between existing set of parameters and new set. 
    bool similar = compare_param_sets(param_sets.front(), _param_set);

    // If param_sets are not similar, throw the existing and ensure
    // regenaration of new access_path.
    if (!similar) {
        is_optimized = false;
        access_paths.clear();
        param_sets.clear();
        param_sets.push_back(_param_set);
    } 

};
 
/*
    Estimates if the parmas between two statements. Should maybe be moved to plan_cache?
*/
bool PLAN_ROOT::compare_param_sets(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2){
    
    // If number of parameters dont match the similarity is not sufficent.
    if (_s1.size() != _s2.size()) return false;
 
    // Compare each param of _s1 and _s2.  
    for (unsigned int i = 0; i < _s1.size(); i++) {
        if (_s1[i].varname != _s2[i].varname) {
            std::cout << "varname not equal: " << std::endl;
            return false;
        }

        if (_s1[i].val != _s2[i].val){
            std::cout << "val not equal: " << std::endl; 
            return false;
        }
        
        if (_s1[i].param_type != _s2[i].param_type) {
            std::cout << "param_type not equal" << std::endl;
            return false;
        } 
            
    }


    /*
    for (unsigned int i = 0; i < _s1.size(); i++) {
        if (*_s1[i].varname->str != *_s2[i].varname->str) {
            std::cout << "varname not equal: " << std::endl;
            return false;
        }
        if (_s1[i].val != _s2[i].val){
            std::cout << "val not equal: " << std::endl;
            return false;
        } 
        if (_s1[i].param_type != _s2[i].param_type) {
            std::cout << "param_type not equal" << std::endl;
            return false;
        } 
            
    }
    */

    return true;
};


/*
void PLAN_ROOT::add_param_set2(std::vector<stmt_param2> _param_set){
    
    // Add param_set if first time stmt is being executed. 

    // Find similarity between existing set of parameters and new set. 
    //bool similar = compare_param_sets(paramsets.front(), _param_set);

    if (param_sets2.size() == 0) {
        param_sets2.push_back(_param_set);
        return;
    }
    std::vector<stmt_param2> _s1 = param_sets2.front();

    for (unsigned int i = 0; i < _s1.size(); i++) {
        if (_s1[i].varname != _param_set[i].varname) {
            std::cout << "varname not equal: " << std::endl;
        }

        if (_s1[i].val != _param_set[i].val){
            std::cout << "val not equal: " << std::endl; 
        }
        
        if (_s1[i].param_type != _param_set[i].param_type) {
            std::cout << "param_type not equal" << std::endl;
        } 
            
    }

    // If param_sets are not similar, throw the existing and ensure
    // regenaration of new access_path.
    param_sets2.push_back(_param_set);

};
*/