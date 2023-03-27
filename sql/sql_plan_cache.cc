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


void PLAN_CACHE::entry(std::string _cache_version, Prepared_statement* _ptr_prep_stmt, 
                       std::vector<stmt_param> _param_set) {
  // Set pointer prepared statement curently being executed.
  set_ptr_prep_stmt(_ptr_prep_stmt);

  // Chech if plan_root with current stmt* already exists.
  bool exists = plan_root_exists(_ptr_prep_stmt);
  
  if (!exists) {
    // Set key to plan_root currently being executed.
    key_active_plan_root = std::make_pair(_ptr_prep_stmt, 1);
    
    // Add plan_root to plan_roots.
    add_plan_root(_param_set);
    return;
  }

  // Init values for switch cases 1 entry.
  PLAN_ROOT* ptr_plan_root;

  // Init values for switch cases N entries.
  int num_entries;
  std::vector<PLAN_ROOT*> plan_root_ptrs;
  //std::vector<stmt_param> param_sets;


  // Switch to correct cache version.
  switch (cache_whitelist[_cache_version]) {
  case EXACT_MATCH_1:
  
    /*
      Retrive and compare params from existing plan_root.
      If inexact match, scrap old access_paths and creat
      new ones. Otherwise, proceed with the same access_paths. 
    */
    key_active_plan_root = std::make_pair(ptr_prep_stmt, 1);
    ptr_plan_root = get_ptr_active_plan_root();
    if (!exact_match(ptr_plan_root->get_param_set(), _param_set)){
      set_optimized_status_plan_root(false);
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
    key_active_plan_root = std::make_pair(ptr_prep_stmt, 1);
    ptr_plan_root = get_ptr_active_plan_root();
    if (!inexact_match(ptr_plan_root->get_param_set(), _param_set)){
      set_optimized_status_plan_root(false);
      ptr_plan_root->clear_access_paths();
      ptr_plan_root->set_param_set(_param_set);
    }
    break;
  case EXACT_MATCH_N:
    /*
    Retrive and compare parmas from existing plan_roots. 

    If none exact match is found and the number of existing plan_roots is less
    than max_num_entries, create a new plan_root and add to plan_roots. 
      
    If none exact match is found and the number of existing plan_roots exceedes
    max_num_entries, use some type of replacement logic to get rid of one of
    the existing plan_roots, before creating a new plan_root and add to plan roots.   
    */

    // Find parmas for existing plan_roots.
    num_entries = num_plan_root_entries(ptr_prep_stmt);
    for (int i = 0; i < num_entries; ++i) {
      auto plan_root = plan_roots.find(std::make_pair(ptr_prep_stmt, i));
      //PLAN_ROOT* ptr = &plan_root->second;
      //std::vector<stmt_param> param_set_helper = ptr->get_param_set();
      plan_root_ptrs.push_back(&plan_root->second);
      //param_sets.push_back(param_set_helper); 
    }

    /*
      Compare _param:_set to exising param_sets. If exact match is found
      set key_active_plan_root and ptr_plan_root AND break process. 
   
    for(unsigned int i = 0; i < plan_root_ptrs.size(); i++){
      if (exact_match(param_sets[i], _param_set)) {
        key_active_plan_root = std::make_pair(ptr_prep_stmt, i);
        ptr_plan_root = get_ptr_active_plan_root();
        break;
      }
    }
    */

    /*
      If none exact match, but num_entries does not excced max_num_entries,
      create new plan_root and add to plan_roots. 
    if (num_entries < max_num_entries) {
      key_active_plan_root = std::make_pair(ptr_prep_stmt, num_entries+1);
      ptr_plan_root = get_ptr_active_plan_root();
      set_optimized_status_plan_root(false);
      ptr_plan_root->clear_access_paths();
      ptr_plan_root->set_param_set(_param_set);
    }
    */

    /*
      If 
    */

  
    break;
  case INEXACT_MATCH_N:
    /*
      Retrive and compare params from existing plan_roots

      If none exact match is found use some cache hit logic to determine if any
      of the existing plan_roots are close enough to be used. 
      
      If none plan_roots are reusable and the existing number of plan_roots is
      less than max_num_entries, create a new plan root and add to plan roots.

      If none plan_roots are reusable and the existing number of plan_roots
      excedes max_num_entries, use some replacement logic to get rid of one
      the existing plan roots, before creating a new plan root and add to plan roots.
    */

    break;
  default:
    break;
  }
};


void PLAN_CACHE::clear_variabels(){
  set_ptr_prep_stmt(nullptr); 
  set_key_active_plan_root(std::make_pair(nullptr, 0));
};

/*
  Removes plan_root from plan_roots and clear plan cache variables. 
  TODO: Delete plan_root object. FOr example with a deconstructor.  
*/
void PLAN_CACHE::cleanup_plan_root(Prepared_statement* _ptr_prep_stmt){
 for (auto it = plan_roots.begin(); it != plan_roots.end(); ) {
    auto key = it->first;
    auto current = it++;
    if (key.first == _ptr_prep_stmt) {
        // Remove plan_root item from plan_roots
        it = plan_roots.erase(current);
    } else {
        it++;
    }
 }
  clear_variabels();
};

void PLAN_CACHE::set_access_path(Query_block* _query_block, AccessPath* _access_path){
   auto plan_root = plan_roots.find(key_active_plan_root);
   plan_root->second.add_access_path(_query_block, _access_path);
};


bool PLAN_CACHE::plan_root_is_optimized(){
  auto plan_root = plan_roots.find(key_active_plan_root);
  return plan_root->second.get_optimized_status();
};

void PLAN_CACHE::set_optimized_status_plan_root(bool _status){
  auto plan_root = plan_roots.find(key_active_plan_root);
  plan_root->second.set_optimized_status(_status);
}

bool PLAN_CACHE::is_executing_prep_stmt(){
  if (ptr_prep_stmt == nullptr) return false;
  return true;
};


void PLAN_CACHE::set_ptr_prep_stmt(Prepared_statement* _ptr_prep_stmt){
  ptr_prep_stmt = _ptr_prep_stmt;
};

void PLAN_CACHE::set_key_active_plan_root(plan_root_key _key){
  key_active_plan_root = _key;
};

plan_root_key PLAN_CACHE::get_key_active_plan_root(){
  return key_active_plan_root;
};

Prepared_statement* PLAN_CACHE::get_ptr_prep_stmt(){
  return ptr_prep_stmt;
};

PLAN_ROOT* PLAN_CACHE::get_ptr_active_plan_root(){
  auto plan_root = plan_roots.find(key_active_plan_root);
  return &plan_root->second;
};


// Count number of versions.
int PLAN_CACHE::num_plan_root_entries(Prepared_statement* _ptr_prep_stmt){
  int counter = 0;
  for (const auto &entry: plan_roots){
    auto plan_root_key = entry.first;
    if (plan_root_key.first == _ptr_prep_stmt) {
      counter++;
    }
  }
  return counter;
};

bool PLAN_CACHE::add_plan_root(std::vector<stmt_param> _param_set){
  // Add new version to plan roots2
    auto it = plan_roots.emplace(key_active_plan_root, PLAN_ROOT(_param_set));
    if (!it.second) return true;
    return false;
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

/*
  Checks if a plan_root with a key containing the parameter 
  _ptr_prep_stmt exists in plan_roots. Returns true if one 
  variant of the plan_root exists, otherwise returns false.  
*/
bool PLAN_CACHE::plan_root_exists(Prepared_statement* _ptr_prep_stmt){
  for (const auto &entry: plan_roots){
    auto plan_root_key = entry.first;
    if (plan_root_key.first == _ptr_prep_stmt) {
        return true;
    }
  }
  return false;
}
