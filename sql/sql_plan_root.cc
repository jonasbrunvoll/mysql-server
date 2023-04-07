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
bool PLAN_ROOT::set_access_path(Query_block* _query_block, AccessPath* _access_path){
    auto it = access_paths.emplace(_query_block, _access_path);
    if (!it.second) return true;
    return false;
};

void PLAN_ROOT::clear_access_paths(){
    access_paths.clear();
};

void PLAN_ROOT::set_param_set(std::vector<stmt_param> _param_set){
    param_set = _param_set;
};

std::vector<stmt_param> PLAN_ROOT::get_param_set(){
    return param_set;
};

void PLAN_ROOT::increment_entries(){
    entries_counter++;
};

void PLAN_ROOT::set_entries(int _entries){
    entries_counter = _entries;
};

int PLAN_ROOT::get_entries(){
    return entries_counter;
};

unsigned int PLAN_ROOT::get_timestamp_created(){
    return timestamp_created;
};

unsigned int PLAN_ROOT::get_timestamp_last_used(){
    return timestamp_last_used;
};

void PLAN_ROOT::set_timestamp_created(){
    timestamp_created = get_timestamp_current_time();
}

void PLAN_ROOT::set_timestamp_last_used(){
    timestamp_last_used = get_timestamp_current_time();
}

// PRIVATE FUNCTIONS //
unsigned int PLAN_ROOT::get_timestamp_current_time(){
    const auto p = std::chrono::system_clock::now();
    return (unsigned int) std::chrono::duration_cast<std::chrono::seconds>(p.time_since_epoch()).count();
}

