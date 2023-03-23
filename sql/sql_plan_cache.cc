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
bool PLAN_CACHE::add_plan_root(std::vector<stmt_param> _param_set) {
  
  // Add new plan_root to plan_roots.
  auto it = plan_roots.emplace(ptr_prep_stmt, PLAN_ROOT(_param_set));
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
bool PLAN_CACHE::plan_root_pair_exists(Prepared_statement* _ptr_prep_stmt){
  return plan_roots.find(_ptr_prep_stmt) != plan_roots.end();
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



void PLAN_CACHE::cleanup_plan_root(Prepared_statement* _ptr_prep_stmt){
  
  /* 
    End execution of func if <Prepared_statment, PLAN_ROOT> pair
    does not exist. Otherwise continue.
  */
  if (!plan_root_pair_exists(_ptr_prep_stmt)) return;

  // Remove item from plan_roots
  plan_roots.erase(_ptr_prep_stmt);

  // Set ptr_prep_stmt to nullpointer. 
  set_ptr_prep_stmt(nullptr);
};

//@return true if item allready exists in plan_cache_dictionary, false otherwise.
bool PLAN_CACHE::plan_root_pair_exists2(Prepared_statement* _ptr_prep_stmt){
  for (const auto &entry: plan_roots2){
    auto key_pair = entry.first;
    if (key_pair.first == _ptr_prep_stmt) return true;
  }
  return false;
};

// Count number of versions.
int PLAN_CACHE::num_versions_plan_root2(Prepared_statement* _ptr_prep_stmt){
  int counter = 0;
  for (const auto &entry: plan_roots2){
    auto key_pair = entry.first;
    if (key_pair.first == _ptr_prep_stmt) {
      counter++;
    }
  }
  return counter;
};

bool PLAN_CACHE::add_plan_root2(Prepared_statement* _ptr_prep_stmt){
  int max = 4;
  key_pair key;

  bool exists = plan_root_pair_exists2(_ptr_prep_stmt);
  
  // new stmt*. Add new <key_pair, PLAN_ROOTS> to plan roots2
  if (!exists){
    key = std::make_pair(_ptr_prep_stmt, 1);
    auto it = plan_roots2.emplace(key, PLAN_ROOT());
    if (!it.second) return true;
    return false;
  }

  

  // stmt* allready exists. Find number of versions.
  int num_versions = num_versions_plan_root2(_ptr_prep_stmt);

  if (num_versions < max){
    /*
      Some logic to fetch and comapre params from other versions...
      ...
      ...

      Fetch params from existing versions. get_parma_set  
      Compare each version after the existing cache hit policies. Switch case?



    */ 

    // Add new version to plan roots2
    key = std::make_pair(_ptr_prep_stmt, num_versions+1);
    auto it = plan_roots2.emplace(key, PLAN_ROOT());
    if (!it.second) return true;
    return false;

  } else {
    // SOME REPLACEMENT LOGIC
  }
  return false;
 };

void PLAN_CACHE::entry(std::string _cache_version, Prepared_statement* _ptr_prep_stmt, 
                       std::vector<stmt_param> _param_set) {
  
  // Set pointer prepared statement curently being executed.
  set_ptr_prep_stmt(_ptr_prep_stmt);


  // Chech if plan_root already exists.
  bool exists = plan_root_pair_exists(_ptr_prep_stmt);

  /*
    Create a new plan_root if the prepared stamtent
    is executed for the first time in this session. 
    The rest of function is aboreted, as the cache versions 
    all handle the first prepred statment similarly.    
  */ 
  if (!exists) {
    add_plan_root(_param_set);
    return;
  }

  
  PLAN_ROOT* ptr_plan_root; 

  std::cout << "cache_version_whitelist[_cache_version]: " << cache_version_whitelist[_cache_version] << std::endl;
  // Switch to correct cache version.
  switch (cache_version_whitelist[_cache_version]) {
  case EXACT_MATCH_1:
  
    /*
      Retrive and compare params from existing plan_root.
      If inexact match, scrap old access_paths and creat
      new ones. Otherwise, proceed with the same access_paths. 
    */
    ptr_plan_root = get_ptr_plan_root();
    if (!exact_match(ptr_plan_root->get_param_set(), _param_set)){
      ptr_plan_root->set_optimized_status(false);
      ptr_plan_root->clear_access_paths();
      ptr_plan_root->set_param_set(_param_set);
    }
    break;
  case INEXACT_MATCH_1:
    /*
      Retrive and compare parmas from existing plan_root. 
      If distance between the param sets are to large, scrap 
      old access_paths and create new ones. Otherwise, 
      proceed with the same access_paths. 
    */
    ptr_plan_root = get_ptr_plan_root();
    if (!inexact_match(ptr_plan_root->get_param_set(), _param_set)){
      ptr_plan_root->set_optimized_status(false);
      ptr_plan_root->clear_access_paths();
      ptr_plan_root->set_param_set(_param_set);
    }
    break;
  case EXACT_MATCH_N:
      /*
      Retrive and compare parmas from existing plan_roots. 

      Create a new version of if none exact match with the existing 
      plan root param sets and number of existing entries is bellow 
      the threshold. 
      
      If none exact match and number of entries are maxed out,
      do some replacment logic to get ride of one of the existing plan root.  
    */
  
    break;
  case INEXACT_MATCH_N:

    break;
  default:
    break;
  }
};
/*
    Estimates if the parmas between two statements.
*/
bool PLAN_CACHE::exact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2){
    
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

/*
  Estimates distance between two param sets. If distance is to large, then
  return false. Otherwise, return true.

  For now shell function which allways returns true.
*/
bool PLAN_CACHE::inexact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2){
    
    if (_s1.size() != _s2.size()) return true;
    return true;
};