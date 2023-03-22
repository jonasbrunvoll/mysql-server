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
bool PLAN_CACHE::add_plan_root() {
  // Add new plan_root to plan_roots.
  auto it = plan_roots.emplace(ptr_prep_stmt, PLAN_ROOT());
  if (!it.second) return true;
  return false;
}

void PLAN_CACHE::set_access_path(Query_block* _query_block, AccessPath* _access_path){
  auto plan_root = plan_roots.find(ptr_prep_stmt);
  plan_root->second.add_access_path(_query_block, _access_path);
};

bool PLAN_CACHE::plan_root_is_optimized(){
  auto plan_root = plan_roots.find(ptr_prep_stmt);
  return plan_root->second.get_optimized_status();
}

void PLAN_CACHE::set_optimized_status_plan_root(bool _status){
  auto plan_root = plan_roots.find(ptr_prep_stmt);
  plan_root->second.set_optimized_status(_status);
}


//@return true if item allready exists in plan_cache_dictionary, false otherwise.
bool PLAN_CACHE::plan_root_pair_exists(Prepared_statement* _ptr_stmt){
  return plan_roots.find(_ptr_stmt) != plan_roots.end();
};


bool PLAN_CACHE::is_executing_prep_stmt(){
  if (ptr_prep_stmt == nullptr) return false;
  return true;
};


void PLAN_CACHE::set_ptr_prep_stmt(Prepared_statement* _ptr_prep_stmt){
  ptr_prep_stmt = _ptr_prep_stmt;
};

Prepared_statement* PLAN_CACHE::get_ptr_prep_stmt(){
  return ptr_prep_stmt;
};

PLAN_ROOT* PLAN_CACHE::get_ptr_plan_root(){
  auto plan_root = plan_roots.find(ptr_prep_stmt);
  return &plan_root->second;
};



void PLAN_CACHE::cleanup_plan_root(Prepared_statement* _ptr_stmt){
  
  /* 
    End execution of func if <Prepared_statment, PLAN_ROOT> pair
    does not exist. Otherwise continue.
  */
  if (!plan_root_pair_exists(_ptr_stmt)) return;

  // Remove item from plan_roots
  plan_roots.erase(_ptr_stmt);

  // Set ptr_prep_stmt to nullpointer. 
  set_ptr_prep_stmt(nullptr);
};


