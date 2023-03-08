#include <iostream>

#include "sql/sql_plan_root.h"
#include "sql/join_optimizer/access_path.h"   // AccessPath
#include "sql/sql_lex.h"                      // Query_block
 
/*
    Add new accesspath* to access_paths. Returns true if existing access_path has
    tha same query_block key as the incoming query_block - access_path pair. Otherwise, 
    returns false after successfully emplacing new access path. 
*/
bool PLAN_ROOT::add_access_path(Query_block* query_block, AccessPath* access_path){
    auto it = access_paths.emplace(query_block, access_path);
    if (!it.second) return true;
    return false;
}

bool PLAN_ROOT::access_path_exists(Query_block* query_block){
    return access_paths.find(query_block) != access_paths.end();
}