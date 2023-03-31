#include <iostream>
#include <stdio.h>
#include <string.h>

#include "sql_class.h"                        // THD, free_items()
#include "sql/sql_plan_cache.h"               // PLAN_CACHE  
#include "sql/sql_plan_root.h"                // PLAN_ROOT
#include "sql/join_optimizer/access_path.h"   // AccessPath
#include "include/my_alloc.h"                 // MEM_ROOT 
#include "sql/thd_raii.h"                     // Swap_mem_root_guard()
#include "sql/sql_lex.h"                      // Query_block

class AccessPath;


void PLAN_CACHE::entry(std::string _match_logic, std::string _entry_logic, std::string _replacement_logic, Prepared_statement* _ptr_prep_stmt, std::vector<stmt_param> _param_set) {
  // Chech if plan_root with current stmt* already exists.
  bool exists = plan_root_exists(_ptr_prep_stmt);
  
  if (!exists) {
    // Set key to plan_root currently being executed.
    key_active_plan_root = std::make_pair(_ptr_prep_stmt, 1);
  
    // Add plan_root to plan_roots.
    add_plan_root(_param_set);
    return;
  }

  // Set pointer to correct match logic function. 
  MatchFunction match_function;

  switch (match_logics[_match_logic]){
  case EXACT_MATCH:
    match_function = &PLAN_CACHE::exact_match;
    break;
  case INEXACT_MATCH:
    match_function = &PLAN_CACHE::inexact_match;
    break;
  case WORST_MATCH:
    match_function = &PLAN_CACHE::exact_match;
    break;
  default:
    break;
  }

  // Init values for switch cases N entries.
  int num_entries;
  std::list<std::vector<stmt_param>> param_sets;


  plan_root_key key_least_used_version;
  int version = -1;
  int num_entries_since_last_used = -1;

  // Entries
  switch (entry_logics[_entry_logic]) {
  case ONE_ENTRY:
    key_active_plan_root = std::make_pair(_ptr_prep_stmt, 1);
    //ptr_plan_root = get_ptr_active_plan_root();
    if (!(this->*match_function)(get_ptr_active_plan_root()->get_param_set(), _param_set)){
      set_optimized_status_plan_root(false);
      get_ptr_active_plan_root()->clear_access_paths();
      get_ptr_active_plan_root()->set_param_set(_param_set);      
    }
    break;
  case N_ENTRIES:
  /*
    Retrive and compare parma sets from existing plan_roots. 

    If none exact match is found and the number of existing plan_roots is less
    than max_num_entries, create a new plan_root and add to plan_roots. 
      
    If none exact match is found and the number of existing plan_roots exceedes
    max_num_entries, use some type of replacement logic to get rid of one of
    the existing plan_roots, before creating a new plan_root and add to plan roots.   
    */
      
    // Find parmas for existing plan_roots.
    num_entries = num_plan_root_entries(_ptr_prep_stmt);

    // Find parmas for existing plan_roots.
     
    for (int i = 1; i <= num_entries; ++i) {
      
      // Fetch plan_root and creat copy of param set.
      auto plan_root = plan_roots.find(std::make_pair(_ptr_prep_stmt, i));
      std::vector<stmt_param> param_set = plan_root->second.get_param_set();

      // If exact match use existing plan_root.
      if (!(this->*match_function)(param_set, _param_set)){
        key_active_plan_root = plan_root->first;
        return;
      }
      // Collect param set for further analysis.
      param_sets.push_back(param_set); 
    }

    /*
      If none exact match, but num_entries does not excced max_num_entries,
      create new plan_root and add to plan_roots. 
    */
    if (num_entries < version_limit) {
      key_active_plan_root = std::make_pair(_ptr_prep_stmt, num_entries+1);
      add_plan_root(_param_set);
      return;
    }
  

    switch (replacement_logics[_replacement_logic]) {
    case FIFO:
      // Erase the first version from plan_roots.
      plan_roots.erase(std::make_pair(_ptr_prep_stmt, 1));
      
      // Move existing versions forward in cache.
      for (int i = 1; i < num_entries; ++i) {
    
        // Fetch plan_root and creat copy of param set.
        auto item = plan_roots.find(std::make_pair(_ptr_prep_stmt, i+1));
        auto key = item->first;
        key.second = key.second - 1;
      }
      
      // Add new version to plan_roots. 
      key_active_plan_root = std::make_pair(_ptr_prep_stmt, num_entries+1);
      add_plan_root(_param_set);
      break;

    case LILO:
       // Erase the last version from plan_roots.
      plan_roots.erase(std::make_pair(_ptr_prep_stmt, num_entries));
      
      // Add new version to plan_roots. 
      key_active_plan_root = std::make_pair(_ptr_prep_stmt, num_entries);
      add_plan_root(_param_set);
      break;

    case LRU: 
      // Move existing versions forward in cache.
      for (int i = 1; i < num_entries; ++i) {
      
        // Fetch plan_root
        auto plan_root = plan_roots.find(std::make_pair(_ptr_prep_stmt, i));
        if (num_entries_since_last_used < plan_root->second.get_entries()){
          num_entries_since_last_used = plan_root->second.get_entries();
          auto key = plan_root->first;
          key_least_used_version = key;
          version = key.second;
        }
      }
      // Remove least recently used plan_root version. 
      plan_roots.erase(key_least_used_version);


      // Add new version to plan_roots. 
      key_active_plan_root = std::make_pair(_ptr_prep_stmt, version);
      add_plan_root(_param_set);

    break;
    default:
      break;
    }

  default:
    break;
  }
};

void PLAN_CACHE::clear_key_active_plan_root(){
  key_active_plan_root = std::make_pair(nullptr, 0);
};


//Removes plan_root from plan_roots and clear plan cache variables. 
void PLAN_CACHE::cleanup_plan_root(THD* thd, Prepared_statement* _ptr_prep_stmt){
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
  thd->free_items();
  clear_key_active_plan_root();
};

void PLAN_CACHE::set_access_path_plan_root(Query_block* _query_block, AccessPath* _access_path){
   auto plan_root = plan_roots.find(key_active_plan_root);
   plan_root->second.set_access_path(_query_block, _access_path);
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
  if (key_active_plan_root.first == nullptr) return false;
  return true;
};

plan_root_key PLAN_CACHE::get_key_active_plan_root(){
  return key_active_plan_root;
};

Prepared_statement* PLAN_CACHE::get_ptr_prep_stmt(){
  return key_active_plan_root.first;
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
  // Add new version to plan roots.
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
};

