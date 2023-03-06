#include <iostream>
#include <stdio.h>
#include <string.h>

#include "sql_class.h"                        // THD
#include "sql/sql_plan_cache.h"               // PLAN_CACHE  
#include "sql/sql_plan_root.h"                // PLAN_ROOT
#include "sql/join_optimizer/access_path.h"   // AccessPath
#include "include/my_alloc.h"                 // MEM_ROOT 
#include "sql/thd_raii.h"                     // Swap_mem_root_guard()

class AccessPath;

bool PLAN_CACHE::add_plan_root(std::string hash_key) {
  bool errorStatus = true;
  unsigned int num_entries = plan_roots.size();
  plan_roots.emplace(hash_key, PLAN_ROOT());
  if (plan_roots.size() > num_entries) errorStatus = false;
  return errorStatus;
};

void PLAN_CACHE::set_access_path(std::string hash_key, AccessPath* ptr_access_path){
   auto plan_root = plan_roots.find(hash_key);
   plan_root->second.path = ptr_access_path;
   
};
/*
* @Return true if hash_key does not exists plan_roots. Otherwise sets 
* thd->mem_root to reference the mem_root of the 
* pl_obj and return false. Should only happen if query is a 
* prepared stmt to be executed. 
*/
bool PLAN_CACHE::swap_mem_root(THD* thd, std::string hash_key){
  if (!plan_root_exists(hash_key)) return true;
  auto plan_root = plan_roots.find(hash_key);
  Swap_mem_root_guard mem_root_guard{thd, &plan_root->second.mem_root};
  return false;
};


//@Return true if empty, false otherwise. 
bool PLAN_CACHE::is_empty(){
  if (plan_roots.size() == 0) return true;
  return false;
};


//@return true if item allready exists in plan_cache_dictionary, false otherwise.
bool PLAN_CACHE::plan_root_exists(std::string hash_key){
  return plan_roots.find(hash_key) != plan_roots.end();
};

// TODO: Implement hash function().
std::string PLAN_CACHE::create_hash_key(std::string query){
  return query;
};

void PLAN_CACHE::set_executing_prep_stmt(){
  this->executing_prep_stmt = !this->executing_prep_stmt;
};

bool PLAN_CACHE::is_executing_prep_stmt(){
  return this->executing_prep_stmt;
};

