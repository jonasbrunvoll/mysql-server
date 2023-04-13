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
#include <fstream>                            // For writing to log file 


class AccessPath;


void PLAN_CACHE::entry(
  std::string _match_logic, 
  std::string _entry_logic, 
  std::string _replacement_logic, 
  Prepared_statement* _ptr_prep_stmt, 
  std::vector<stmt_param> _param_set
){
  // Chech if plan_root with current stmt* already exists.
  bool exists = plan_root_exists(_ptr_prep_stmt);
  
  if (!exists) {
    // Perform replacement of a stored plan root if cache limit is execeeded. 
    std::vector<plan_root_key> versions;
    global_replacement(_replacement_logic,_ptr_prep_stmt,_param_set, versions);

    // Set key to plan_root currently being executed.
    key_active_plan_root = std::make_pair(_ptr_prep_stmt, 1);
  
    // Add plan_root to plan_roots.
    add_plan_root(_param_set);
    return;
  }
  
  // Switch case determining replacement logic based on number of entries. 
  switch (entry_logics[_entry_logic]) {
    case ONE_ENTRY:{
      // Set key to plan_root currently being executed.
      key_active_plan_root = std::make_pair(_ptr_prep_stmt, 1);
      get_ptr_active_plan_root()->set_timestamp_last_used();

      // If inexact match, continue execution by reusing existing execution plan. 
      if (match_logics[_match_logic] == INEXACT_MATCH) break;

      // Break if exact match between param sets.     
      if (exact_match(_param_set, get_ptr_active_plan_root()->get_param_set())) break;
      
      // Re-optimize prepared statment. 
      set_optimized_status_plan_root(false);
      get_ptr_active_plan_root()->clear_access_paths();
      get_ptr_active_plan_root()->set_param_set(_param_set);      
      break;
    }

    /*
      Retrive and compare parma sets from existing plan_roots. 

      If none exact match is found and the number of existing plan_roots is less
      than max_num_entries, create a new plan_root and add to plan_roots. 
        
      If none exact match is found and the number of existing plan_roots exceedes
      max_num_entries, use some type of replacement logic to get rid of one of
      the existing plan_roots, before creating a new plan_root and add to plan roots.   
    */
    case N_ENTRIES:{      

      // Fetch versions
      std::vector<plan_root_key> version_keys = get_version_keys(_ptr_prep_stmt);

      for (auto const &key: version_keys){
        auto plan_root = plan_roots.find(key);
        if (plan_root == plan_roots.end()) break;
        std::vector<stmt_param> param_set = plan_root->second.get_param_set();

        // If exact match use existing plan_root.
        if (exact_match(param_set, _param_set)){
          key_active_plan_root = plan_root->first;
          get_ptr_active_plan_root()->set_timestamp_last_used();
          return;
        }
      }
      
      // Perform replacement of a stored plan root if cache limit is execeeded. 
      global_replacement(_replacement_logic,_ptr_prep_stmt,_param_set, version_keys);

      /*
        If none match, but num_entries does not excced max_num_entries,
        create new plan_root and add to plan_roots. 
      */
      if (version_keys.size() < version_limit) {
        key_active_plan_root = std::make_pair(_ptr_prep_stmt, version_keys.size()+1);
        add_plan_root(_param_set);
        return;
      }

      version_replacement(_replacement_logic, _ptr_prep_stmt, _param_set, version_keys);
    }
    default:{
      break;
    }
  }
};

void PLAN_CACHE::version_replacement(
  std::string _replacement_logic, 
  Prepared_statement* _ptr_prep_stmt, 
  std::vector<stmt_param> _param_set, 
  std::vector<plan_root_key> _version_keys
){
  switch (replacement_logics[_replacement_logic]) {
      case FIFO:{
        plan_root_key curr_key;
        unsigned int timestamp = INT_MAX;

        for (auto const &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          curr_key = plan_root->first;
          if(timestamp > get_ptr_plan_root(curr_key)->get_timestamp_created()){
            timestamp = get_ptr_plan_root(curr_key)->get_timestamp_created();
          }
        }

        // Erase the first version from plan_roots.
        erase_plan_root(curr_key);
        //plan_roots.erase(curr_key);

        // Add new version to plan_roots. 
        key_active_plan_root = curr_key;
        add_plan_root(_param_set);
        break; 
      }
      case LILO:{
        plan_root_key curr_key;
        unsigned int timestamp = 0;

        for (auto const &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          curr_key = plan_root->first;
          if(timestamp < get_ptr_plan_root(curr_key)->get_timestamp_created()){
            timestamp = get_ptr_plan_root(curr_key)->get_timestamp_created();
          }
        }

        // Erase the first version from plan_roots.
        erase_plan_root(curr_key);
        //plan_roots.erase(curr_key);

        // Add new version to plan_roots. 
        key_active_plan_root = curr_key;
        add_plan_root(_param_set);
        break;
      }
      /*
        Iterate over all version of prepared stmt and fetch 
        the entry counters. The least recently used plan root is
        erased and a new plan root is
      */ 
      case LRU:{
        plan_root_key curr_key;
        unsigned int timestamp = INT_MAX;

        for (auto const &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          curr_key = plan_root->first;
          if(timestamp > get_ptr_plan_root(curr_key)->get_timestamp_last_used()){
            timestamp = get_ptr_plan_root(curr_key)->get_timestamp_last_used();
          }
        }

        // Remove least recently used plan_root version.
        erase_plan_root(curr_key); 
        //plan_roots.erase(curr_key);

        // Add new version to plan_roots. 
        key_active_plan_root = curr_key;
        add_plan_root(_param_set);
        break;
      }
      case WORST_MATCH:{
        // Compare _param_set against each param set in param sets.
        plan_root_key current_key;
        unsigned int most_unsimilar_params = 0;
        
        for (auto const &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          unsigned int counter = 0;
          std::vector<stmt_param> fetched_param_set = plan_root->second.get_param_set();

          if (_param_set.size() != fetched_param_set.size()) {
            counter += _param_set.size();
          } else {
            for (unsigned int i = 0; i < _param_set.size(); i++){
              if (
                _param_set[i].param_type != fetched_param_set[i].param_type 
                || _param_set[i].varname != fetched_param_set[i].varname 
                || _param_set[i].val != fetched_param_set[i].val
                ) {
                counter++;
              } 
            }
            if (most_unsimilar_params < counter) {
              most_unsimilar_params = counter;
              current_key = key;
            } 
          }
        }
        // Remove least recently used plan_root version. 
        erase_plan_root(current_key);
        //plan_roots.erase(current_key);

        // Add new version to plan_roots. 
        key_active_plan_root = std::make_pair(_ptr_prep_stmt, current_key.second); 
        add_plan_root(_param_set);
        break;
      }
      default:{
        break;
      }
    }
};

void PLAN_CACHE::global_replacement(
  std::string _replacement_logic, 
  Prepared_statement* _ptr_prep_stmt,
  std::vector<stmt_param> _param_set,
  std::vector<plan_root_key> _version_keys
){

  // Retrun if cache there is space for one ore more plan roots in plan_cache. 
  if (plan_roots.size() < cache_limit) return;
  
  switch (replacement_logics[_replacement_logic]) {
    case FIFO:{
      plan_root_key key;
      unsigned int timestamp = INT_MAX;
      for (auto &it: plan_roots){   
         if(timestamp > it.second.get_timestamp_created()){
            key = it.first;
            timestamp = it.second.get_timestamp_created();
          }
      }
      // Erase the first plan root in from plan_roots.
      erase_plan_root(key);
      //plan_roots.erase(key);            
      break;
    }
    case LILO:{
      plan_root_key key;
      unsigned int timestamp = 0;
      for (auto &it: plan_roots){   
         if(timestamp < it.second.get_timestamp_created()){
            key = it.first;
            timestamp = it.second.get_timestamp_created();
          }
      }
       // Erase the last plan root added to plan_roots.
      erase_plan_root(key);
      //plan_roots.erase(key);
      break;
    }
    case LRU:{
      plan_root_key key;
      unsigned int timestamp = INT_MAX;
      for (auto &it: plan_roots){   
         if(timestamp > it.second.get_timestamp_last_used()){
            key = it.first;
            timestamp = it.second.get_timestamp_last_used();
          }
      }
      // Remove least recently used plan_root. 
      erase_plan_root(key);
      //plan_roots.erase(key);    
      break;
    } 
    case WORST_MATCH:{
      /*
        If there allready exists at plan root object(s) with the same *stmt key-value.
        the least similar version will be erased. Otherwise, the least similar overal 
        plan root will be erased.  
      */ 

      if (_version_keys.size() != 0){
   
        // Compare _param_set against each param set in param sets.    
        plan_root_key current_key;
        unsigned int most_unsimilar_params = 0;
        
        for (auto &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          unsigned int counter = 0;
          std::vector<stmt_param> fetched_param_set = plan_root->second.get_param_set();

          if (_param_set.size() != fetched_param_set.size()) {
            counter += _param_set.size();
          } else {
            for (unsigned int i = 0; i < _param_set.size(); i++){
              if (
                _param_set[i].param_type != fetched_param_set[i].param_type 
                || _param_set[i].varname != fetched_param_set[i].varname 
                || _param_set[i].val != fetched_param_set[i].val
                ) {
                counter++;
              } 
            }
            if (most_unsimilar_params < counter) {
              most_unsimilar_params = counter;
              current_key = key;
            } 
          }
        }
        // Remove least recently used plan_root version.
        erase_plan_root(current_key); 
        //plan_roots.erase(current_key);

        // Add new version to plan_roots. 
        key_active_plan_root = std::make_pair(_ptr_prep_stmt, current_key.second); 
      } else {
        // Compare _param_set against each param set in param sets.    
        plan_root_key current_key;
        unsigned int most_unsimilar_params = 0;
        
        for (auto &it: plan_roots){
          unsigned int counter = 0;
          std::vector<stmt_param> fetched_param_set = it.second.get_param_set();

          if (_param_set.size() != fetched_param_set.size()) {
            counter += _param_set.size();
          } else {
            for (unsigned int i = 0; i < _param_set.size(); i++){
              if (
                _param_set[i].param_type != fetched_param_set[i].param_type 
                || _param_set[i].varname != fetched_param_set[i].varname 
                || _param_set[i].val != fetched_param_set[i].val
                ) {
                counter++;
              } 
            }
          }

          if (most_unsimilar_params < counter) {
            most_unsimilar_params = counter;
            current_key = it.first;
          } 
        }
        // Remove least recently used plan_root version. 
        erase_plan_root(current_key);
        //plan_roots.erase(current_key);
        
        // Add new version to plan_roots. 
        key_active_plan_root = std::make_pair(_ptr_prep_stmt, 1);
      }
      add_plan_root(_param_set);
      break;
    }
      default:{
        break;
      }
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


void PLAN_CACHE::clear_key_active_plan_root(){
  key_active_plan_root = std::make_pair(nullptr, 0);
};


//Removes plan_root from plan_roots and clear plan cache variables. 
void PLAN_CACHE::cleanup_plan_root(THD* thd, Prepared_statement* _ptr_prep_stmt){
 for (auto it = plan_roots.begin(); it != plan_roots.end(); ) {
    auto key = it->first;
    auto current = it++;
    if (key.first == _ptr_prep_stmt) {
        // Clenup temp_tabls
        get_ptr_plan_root(key)->cleanup_temp_table_ptrs();

        
        // Remove plan_root item from plan_roots
        it = plan_roots.erase(current);
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
  if (plan_root == plan_roots.end()) return false;
  return plan_root->second.get_optimized_status();
};

void PLAN_CACHE::set_optimized_status_plan_root(bool _status){
  auto plan_root = plan_roots.find(key_active_plan_root);
  if (plan_root == plan_roots.end()) return;
  plan_root->second.set_optimized_status(_status);
}

bool PLAN_CACHE::is_executing_prep_stmt(){
  if (key_active_plan_root.first == nullptr) return false;
  return true;
};

Prepared_statement* PLAN_CACHE::get_ptr_prep_stmt(){
  return key_active_plan_root.first;
};

PLAN_ROOT* PLAN_CACHE::get_ptr_active_plan_root(){
  auto plan_root = plan_roots.find(key_active_plan_root);
  return &plan_root->second;
};


PLAN_ROOT* PLAN_CACHE::get_ptr_plan_root(plan_root_key _key){
  auto plan_root = plan_roots.find(_key);
  if (plan_root == plan_roots.end()) return nullptr;
  return &plan_root->second;
};


void PLAN_CACHE::log_results(
  std::clock_t _duration_opt, 
  std::clock_t _duration_exec, 
  bool _prepared_statment, 
  std::string _query_string){

  // Cast to ms
  //dur_opt = dur_opt / (double)(CLOCKS_PER_SEC);
  //dur_exec = dur_exec / (double)(CLOCKS_PER_SEC);
  //dur_opt = dur_opt / (double)(CLOCKS_PER_SEC / 1000);
  //dur_exec = dur_exec / (double)(CLOCKS_PER_SEC / 1000);

  const char *path="/home/jonas/mysql/experiments/log.text";
  std::ofstream logFile;
  logFile.open(path, std::ios_base::app);
  logFile << _duration_opt << "," << _duration_exec << "," << _prepared_statment << "," << _query_string << std::endl;
  std::cout << _duration_opt << "," << _duration_exec << "," << _prepared_statment << "," << _query_string << std::endl;
  //std::cerr <<  "Is prepared statment: " << isPrepared << " dur_opt: " << dur_opt << "ms. dur_exec: " << dur_exec << "ms." << std::endl;
};

bool PLAN_CACHE::add_plan_root(std::vector<stmt_param> _param_set){
  // Add new version to plan roots.
  auto it = plan_roots.emplace(key_active_plan_root, PLAN_ROOT(_param_set));
  if (!it.second) return true;
  return false;
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


std::vector<plan_root_key> PLAN_CACHE::get_version_keys(Prepared_statement* _ptr_prep_stmt){
  std::vector<plan_root_key> versions;
  for (const auto &it: plan_roots){
    auto plan_root_key = it.first;
    if (plan_root_key.first == _ptr_prep_stmt) {
      versions.push_back(plan_root_key);
    }
  }
  return versions;
};


void PLAN_CACHE::erase_plan_root(plan_root_key _key_plan_root){
  get_ptr_plan_root(_key_plan_root)->cleanup_temp_table_ptrs();
  plan_roots.erase(_key_plan_root);
};

/*
  Clean up all temp tables generated through executiopn
  of all plan root objects.  
*/
void PLAN_CACHE::cleanup_tmp_tables(){

  if (plan_roots.empty()) return;

  for(auto const& plan_root : plan_roots){
    get_ptr_plan_root(plan_root.first)->cleanup_temp_table_ptrs();
  }

  
  plan_roots.clear();  
};
