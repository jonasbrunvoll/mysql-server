#include <iostream>
#include <stdio.h>
#include <string.h>

#include "sql_class.h"                        // THD
#include "sql/sql_plan_cache.h"               // PLAN_CACHE  
#include "sql/sql_plan_root.h"                // PLAN_ROOT
#include "sql/join_optimizer/access_path.h"   // AccessPath
#include "include/my_alloc.h"                 // MEM_ROOT 
#include "sql/thd_raii.h"                     // Swap_mem_root_guard()
#include "sql/sql_lex.h"                      // Query_block


class AccessPath;
bool PLAN_CACHE::add_plan_root2() {
  // Add new plan_root to plan_roots.
  auto it = plan_roots.emplace(this->ptr_prep_stmt, PLAN_ROOT());
  if (!it.second) return true;
  return false;
}

/*
* @ Return true if error, otherwise false.
*/
bool PLAN_CACHE::add_plan_root(Query_block* query_block, AccessPath* access_path) {
  // Add new plan_root to plan_roots.
  auto it = plan_roots.emplace(this->ptr_prep_stmt, PLAN_ROOT());
  if (!it.second) return true;

  // Find plan_root and set access_path.
  //auto plan_root = plan_roots.find(this->ptr_prep_stmt);
  //plan_root->second.path = access_path;
  
  auto plan_root = plan_roots.find(this->ptr_prep_stmt);
  plan_root->second.add_access_path(query_block, access_path);

  
  return false; 
};

void PLAN_CACHE::set_access_path(Query_block* query_block, AccessPath* access_path){
  auto plan_root = plan_roots.find(this->ptr_prep_stmt);
  plan_root->second.add_access_path(query_block, access_path);
};

bool PLAN_CACHE::plan_root_is_optimized(){
  auto plan_root = plan_roots.find(this->ptr_prep_stmt);
  return plan_root->second.is_optimized;
}

void PLAN_CACHE::plan_root_set_optimized(){
  auto plan_root = plan_roots.find(this->ptr_prep_stmt);
  plan_root->second.is_optimized = true;
}


/*
* @Return true if hash_key does not exists plan_roots. Otherwise sets 
* thd->mem_root to reference the mem_root of the 
* pl_obj and return false. Should only happen if query is a 
* prepared stmt to be executed. 
*/
bool PLAN_CACHE::swap_mem_root(THD* thd){
  if (!plan_root_exists()) return true;
  auto plan_root = plan_roots.find(this->ptr_prep_stmt);
  Swap_mem_root_guard mem_root_guard{thd, &plan_root->second.mem_root};
  return false;
};


//@return true if item allready exists in plan_cache_dictionary, false otherwise.
bool PLAN_CACHE::plan_root_exists(){
  return plan_roots.find(this->ptr_prep_stmt) != plan_roots.end();
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


void PLAN_CACHE::set_ptr_prep_stmt(Prepared_statement* ptr_prep_stmt){
  this->ptr_prep_stmt = ptr_prep_stmt;
};

Prepared_statement* PLAN_CACHE::get_ptr_prep_stmt(){
  return this->ptr_prep_stmt;
};

PLAN_ROOT* PLAN_CACHE::get_ptr_plan_root(){
  auto plan_root = plan_roots.find(this->ptr_prep_stmt);
  return &plan_root->second;
};