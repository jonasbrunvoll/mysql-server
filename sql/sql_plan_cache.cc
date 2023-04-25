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


void PLAN_CACHE::enter_plan_cache(
  std::string _match_logic, 
  std::string _entry_logic, 
  std::string _replacement_logic, 
  Prepared_statement* _prepared_statement, 
  std::vector<prepared_statement_parameter> _parameters
){
  // Chech if plan_root with current stmt* already exists.
  bool exists = plan_root_exists(_prepared_statement);
  
  if (!exists) {
    // Perform replacement of a stored plan root if cache limit is execeeded. 
    std::vector<plan_root_key> _version_keys;
    global_replacement(_replacement_logic,_prepared_statement,_parameters, _version_keys);

    // Set key to plan_root currently being executed.
    active_plan_root_key = std::make_pair(_prepared_statement, 1);
  
    // Add plan_root to plan_roots.
    add_plan_root(_parameters);
    return;
  }
  
  // Switch case determining replacement logic based on number of entries. 
  switch (entry_logics[_entry_logic]) {
    case ONE_ENTRY:{
      // Set key to plan_root currently being executed.
      active_plan_root_key = std::make_pair(_prepared_statement, 1);
      get_active_plan_root()->set_timestamp_last_accessed();

      // If inexact match, continue execution by reusing existing execution plan. 
      if (match_logics[_match_logic] == INEXACT_MATCH) break;

      // Break if exact match between param sets.     
      if (exact_match(_parameters, get_active_plan_root()->get_parameters())) break;
      
      // Re-optimize prepared statment. 
      set_optimized_status_plan_root(false);
      get_active_plan_root()->clear_access_paths();
      get_active_plan_root()->set_parameters(_parameters);      
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
      std::vector<plan_root_key> version_keys = get_version_keys_prepared_statement(_prepared_statement);

      for (auto const &key: version_keys){
        auto plan_root = plan_roots.find(key);
        if (plan_root == plan_roots.end()) break;
        std::vector<prepared_statement_parameter> existing_parameters = plan_root->second.get_parameters();

        // If exact match use existing plan_root.
        if (exact_match(_parameters, existing_parameters)){
          active_plan_root_key = plan_root->first;
          get_active_plan_root()->set_timestamp_last_accessed();
          return;
        }
      }
      
      // Perform replacement of a stored plan root if cache limit is execeeded. 
      global_replacement(_replacement_logic,_prepared_statement,_parameters, version_keys);

      /*
        If none match, but num_entries does not excced max_num_entries,
        create new plan_root and add to plan_roots. 
      */
      if (version_keys.size() < version_limit) {
        active_plan_root_key = std::make_pair(_prepared_statement, version_keys.size()+1);
        add_plan_root(_parameters);
        return;
      }

      version_replacement(_replacement_logic, _prepared_statement, _parameters, version_keys);
    }
    default:{
      break;
    }
  }
};

void PLAN_CACHE::version_replacement(
  std::string _replacement_logic, 
  Prepared_statement* _prepared_statement, 
  std::vector<prepared_statement_parameter> _parameters, 
  std::vector<plan_root_key> _version_keys
){
  switch (replacement_logics[_replacement_logic]) {
      case FIFO:{
        plan_root_key plan_root_key;
        unsigned int timestamp = INT_MAX;

        for (auto const &key: _version_keys){
        
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          
          plan_root_key = plan_root->first;
          if(timestamp > get_plan_root(plan_root_key)->get_timestamp_created()){
            timestamp = get_plan_root(plan_root_key)->get_timestamp_created();
          }
        }
        // Erase the first version from plan_roots.
        erase_plan_root(plan_root_key);

        // Add new version to plan_roots. 
        active_plan_root_key = plan_root_key;
        add_plan_root(_parameters);
        break; 
      }
      case LIFO:{
        plan_root_key plan_root_key;
        unsigned int timestamp = 0;

        for (auto const &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          plan_root_key = plan_root->first;
          if(timestamp < get_plan_root(plan_root_key)->get_timestamp_created()){
            timestamp = get_plan_root(plan_root_key)->get_timestamp_created();
          }
        }
        // Erase the first version from plan_roots.
        erase_plan_root(plan_root_key);

        // Add new version to plan_roots. 
        active_plan_root_key = plan_root_key;
        add_plan_root(_parameters);
        break;
      }
      /*
        Iterate over all version of prepared stmt and fetch 
        the entry counters. The least recently used plan root is
        erased and a new plan root is
      */ 
      case LRU:{
        plan_root_key plan_root_key;
        unsigned int timestamp = INT_MAX;

        for (auto const &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          plan_root_key = plan_root->first;
          if(timestamp > get_plan_root(plan_root_key)->get_timestamp_last_accessed()){
            timestamp = get_plan_root(plan_root_key)->get_timestamp_last_accessed();
          }
        }
        // Remove least recently used plan_root version.
        erase_plan_root(plan_root_key); 
        
        // Add new version to plan_roots. 
        active_plan_root_key = plan_root_key;
        add_plan_root(_parameters);
        break;
      }
      case WORST_MATCH:{
        // Compare _param_set against each param set in param sets.
        plan_root_key plan_root_key;
        unsigned int most_unsimilar_parameters = 0;
        
        for (auto const &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          unsigned int counter = 0;
          std::vector<prepared_statement_parameter> existing_parameters = plan_root->second.get_parameters();

          if (_parameters.size() != existing_parameters.size()) {
            counter += _parameters.size();
          } else {
            for (unsigned int i = 0; i < _parameters.size(); i++){
              if (_parameters[i].param_type != existing_parameters[i].param_type 
                || _parameters[i].varname != existing_parameters[i].varname 
                || _parameters[i].val != existing_parameters[i].val
              ) counter++;
            }

            if (most_unsimilar_parameters < counter) {
              most_unsimilar_parameters = counter;
              plan_root_key = key;
            } 
          }
        }
        // Remove least recently used plan_root version. 
        erase_plan_root(plan_root_key);

        // Add new version to plan_roots. 
        active_plan_root_key = std::make_pair(_prepared_statement, plan_root_key.second); 
        add_plan_root(_parameters);
        break;
      }
      default:{
        break;
      }
    }
};

void PLAN_CACHE::global_replacement(
  std::string _replacement_logic, 
  Prepared_statement* _prepared_statement,
  std::vector<prepared_statement_parameter> _parameters,
  std::vector<plan_root_key> _version_keys
){

  // Retrun if cache there is space for one ore more plan roots in plan_cache. 
  if (plan_roots.size() < global_limit) return;
  
  switch (replacement_logics[_replacement_logic]) {
    case FIFO:{
      plan_root_key plan_root_key;
      unsigned int timestamp = INT_MAX;
      for (auto &plan_root: plan_roots){   
         if(timestamp > plan_root.second.get_timestamp_created()){
            plan_root_key = plan_root.first;
            timestamp = plan_root.second.get_timestamp_created();
          }
      }
      // Erase the first plan root in from plan_roots.
      erase_plan_root(plan_root_key);          
      break;
    }
    case LIFO:{
      plan_root_key plan_root_key;
      unsigned int timestamp = 0;
      for (auto &plan_root: plan_roots){   
         if(timestamp < plan_root.second.get_timestamp_created()){
            plan_root_key = plan_root.first;
            timestamp = plan_root.second.get_timestamp_created();
          }
      }
       // Erase the last plan root added to plan_roots.
      erase_plan_root(plan_root_key);
      break;
    }
    case LRU:{
      plan_root_key plan_root_key;
      unsigned int timestamp = INT_MAX;
      for (auto &plan_root: plan_roots){   
         if(timestamp > plan_root.second.get_timestamp_last_accessed()){
            plan_root_key = plan_root.first;
            timestamp = plan_root.second.get_timestamp_last_accessed();
          }
      }
      // Remove least recently used plan_root. 
      erase_plan_root(plan_root_key);
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
        plan_root_key plan_root_key;
        unsigned int most_unsimilar_params = 0;
        
        for (auto &key: _version_keys){
          auto plan_root = plan_roots.find(key);
          if (plan_root == plan_roots.end()) break;
          unsigned int counter = 0;
          std::vector<prepared_statement_parameter> fetched_param_set = plan_root->second.get_parameters();

          if (_parameters.size() != fetched_param_set.size()) {
            counter += _parameters.size();
          } else {
            for (unsigned int i = 0; i < _parameters.size(); i++){
              if (_parameters[i].param_type != fetched_param_set[i].param_type 
                || _parameters[i].varname != fetched_param_set[i].varname 
                || _parameters[i].val != fetched_param_set[i].val
              ) counter++;
            }
            if (most_unsimilar_params < counter) {
              most_unsimilar_params = counter;
              plan_root_key = key;
            } 
          }
        }
        // Remove least recently used plan_root version.
        erase_plan_root(plan_root_key); 

        // Add new version to plan_roots. 
        active_plan_root_key = std::make_pair(_prepared_statement, plan_root_key.second); 
      } else {
        // Compare _param_set against each param set in param sets.    
        plan_root_key plan_root_key;
        int most_unsimilar_params = -1;
        
        for (auto &plan_root: plan_roots){
          int counter = 0;
          std::vector<prepared_statement_parameter> fetched_param_set = plan_root.second.get_parameters();

          if (_parameters.size() != fetched_param_set.size()) {
            counter += _parameters.size();
          } else {
            for (unsigned int i = 0; i < _parameters.size(); i++){
              if (
                _parameters[i].param_type != fetched_param_set[i].param_type 
                || _parameters[i].varname != fetched_param_set[i].varname 
                || _parameters[i].val != fetched_param_set[i].val
                ) counter++;
            }
          }

          if (most_unsimilar_params < counter) {
            most_unsimilar_params = counter;
            plan_root_key = plan_root.first;
          } 
        }
        // Remove least recently used plan_root version. 
        erase_plan_root(plan_root_key);
        
        // Add new version to plan_roots. 
        active_plan_root_key = std::make_pair(_prepared_statement, 1);
      }
      add_plan_root(_parameters);
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
bool PLAN_CACHE::exact_match(std::vector <prepared_statement_parameter> _parameters_x, std::vector <prepared_statement_parameter> _parameters_y){
    
    // If the number of parameters do not match the similarity is not sufficent.
    if (_parameters_x.size() != _parameters_y.size()) return false;
 
    // Compare the parameters from _parameters_x and _parameters_y   
    for (unsigned int i = 0; i < _parameters_x.size(); i++) {
        if (_parameters_x[i].param_type != _parameters_y[i].param_type) return false;
        if (_parameters_x[i].varname != _parameters_y[i].varname) return false;
        if (_parameters_x[i].val != _parameters_y[i].val) return false;
    }
    return true;
};


void PLAN_CACHE::clear_active_plan_root_key(){
  active_plan_root_key = std::make_pair(nullptr, 0);
};


// Removes plan_root from plan_roots and clear plan cache variables. 
void PLAN_CACHE::remove_plan_root(THD* thd, Prepared_statement* _prepared_statement){
 for (auto plan_root = plan_roots.begin(); plan_root != plan_roots.end(); ) {
    auto plan_root_key = plan_root->first;
    auto current_plan_root = plan_root++;
    
    if (plan_root_key.first == _prepared_statement) {
      get_plan_root(plan_root_key)->free_temp_tables();

      // Remove plan_root from plan_roots
      plan_root = plan_roots.erase(current_plan_root);
    } 
 }
  thd->free_items();
  clear_active_plan_root_key();
};

void PLAN_CACHE::set_access_path_plan_root(Query_block* _query_block, AccessPath* _access_path){
   auto plan_root = plan_roots.find(active_plan_root_key);
   plan_root->second.add_access_path(_query_block, _access_path);
};


bool PLAN_CACHE::plan_root_is_optimized(){
  auto plan_root = plan_roots.find(active_plan_root_key);
  if (plan_root == plan_roots.end()) return false;
  return plan_root->second.is_optimized();
};

void PLAN_CACHE::set_optimized_status_plan_root(bool _status){
  auto plan_root = plan_roots.find(active_plan_root_key);
  if (plan_root == plan_roots.end()) return;
  plan_root->second.set_optimized_status(_status);
}

bool PLAN_CACHE::executes_prepared_statment(){
  if (active_plan_root_key.first == nullptr) return false;
  return true;
};


PLAN_ROOT* PLAN_CACHE::get_active_plan_root(){
  auto plan_root = plan_roots.find(active_plan_root_key);
  return &plan_root->second;
};


PLAN_ROOT* PLAN_CACHE::get_plan_root(plan_root_key _plan_root_key){
  auto plan_root = plan_roots.find(_plan_root_key);
  if (plan_root == plan_roots.end()) return nullptr;
  return &plan_root->second;
};


std::string PLAN_CACHE::format_varchar(enum_field_types _field_type, std::string _parameter){
  if (_field_type != MYSQL_TYPE_VARCHAR) return _parameter;
  
  // If data_type is varchar, get rid of all chars except the string value between ' '.
  unsigned index_first_occurance = _parameter.find("'");
  unsigned index_last_occurance = _parameter.find_last_of("'");
  return _parameter.substr(
    index_first_occurance + 1,
    index_last_occurance-1-index_first_occurance
  );
}


void PLAN_CACHE::log_time_consumption(
  std::clock_t _duration_opt, 
  std::clock_t _duration_exec, 
  bool _prepared_statment, 
  std::string _query_string){
  
  const char *path="/home/jonas/mysql/experiments/log.txt";
  std::ofstream logFile;
  logFile.open(path, std::ios_base::app);

  // Micro seconds (10^-6) SECONDS
  //double durations_exe_after_opt_ms = (_duration_exec - _duration_opt)/(CLOCKS_PER_SEC/1000);
  //double duration_opt_ms=(_duration_opt)/(CLOCKS_PER_SEC/1000);
  //logFile << duration_opt_ms << "," << durations_exe_after_opt_ms << "," << _prepared_statment << "," << _query_string << std::endl;
  logFile << _duration_opt << "," << _duration_exec << "," << _prepared_statment << "," << _query_string << std::endl;
};

bool PLAN_CACHE::add_plan_root(std::vector<prepared_statement_parameter> _parameters){
  // Add new version to plan roots.
  auto plan_root = plan_roots.emplace(active_plan_root_key, PLAN_ROOT(_parameters));
  if (!plan_root.second) return true;
  return false;
};

/*
  Checks if a plan_root with a key containing the parameter 
  _ptr_prep_stmt exists in plan_roots. Returns true if one 
  variant of the plan_root exists, otherwise returns false.  
*/
bool PLAN_CACHE::plan_root_exists(Prepared_statement* _prepared_statement){
  for (const auto &entry: plan_roots){
    auto plan_root_key = entry.first;
    if (plan_root_key.first == _prepared_statement) {
        return true;
    }
  }
  return false;
};


std::vector<plan_root_key> PLAN_CACHE::get_version_keys_prepared_statement(Prepared_statement* _prepared_statement){
  std::vector<plan_root_key> version_keys;
  for (const auto &plan_root: plan_roots){
    auto plan_root_key = plan_root.first;
    if (plan_root_key.first == _prepared_statement) {
      version_keys.push_back(plan_root_key);
    }
  }
  return version_keys;
};


void PLAN_CACHE::erase_plan_root(plan_root_key _key_plan_root){
  get_plan_root(_key_plan_root)->free_temp_tables();
  plan_roots.erase(_key_plan_root);
};

/*
  Clean up all temp tables generated through executiopn
  of all plan root objects.  
*/
void PLAN_CACHE::free_all_tmp_tables(){

  if (plan_roots.empty()) return;

  for(auto const& plan_root : plan_roots){
    get_plan_root(plan_root.first)->free_temp_tables();
  }  
  plan_roots.clear();  
};
