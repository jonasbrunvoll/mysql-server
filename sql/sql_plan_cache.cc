#include <iostream>
#include <stdio.h>
#include <string.h>

#include "sql/sql_plan_cache.h"
#include "sql/sql_plan_cache_item.h"
#include "sql/join_optimizer/access_path.h"
#include "include/my_alloc.h"

class AccessPath;

/*
bool PLAN_CACHE::insert_test_dict(std::string key, AccessPath path){
  bool errorStatus = false;
  unsigned int num_entries = test_dict.size();
  MEM_ROOT *root;
  AccessPath *path = new (root) AccessPath;
  *path = path;
  test_dict.insert(std::pair<std::string, *MEM_ROOT>(key, root));
  if (plan_cache_dictionary.size() > num_entries) errorStatus = false;
  return errorStatus;
};
*/


bool PLAN_CACHE::insert_item(std::string key, PLAN_CACHE_ITEM item) {
  bool errorStatus = true;
  unsigned int num_entries = plan_cache_dictionary.size();
  plan_cache_dictionary.insert(std::pair<std::string, PLAN_CACHE_ITEM>(key, item));
  if (plan_cache_dictionary.size() > num_entries) errorStatus = false;
  return errorStatus;
};


bool PLAN_CACHE::remove_item(std::string key){
  bool errorStatus = true;
  unsigned int num_entries = plan_cache_dictionary.size();
  plan_cache_dictionary.erase(key);
  if (plan_cache_dictionary.size() < num_entries) errorStatus = false;
  return errorStatus;
};


//@Return true if empty, false otherwise. 
bool PLAN_CACHE::is_empty(){
  if (plan_cache_dictionary.size() == 0) return true;
  return false;
};


//@return true if item allready exists in plan_cache_dictionary, false otherwise.
bool PLAN_CACHE::contains_item(std::string key){
  return plan_cache_dictionary.find(key) != plan_cache_dictionary.end();
}

// TODO: Implement hash function().
std::string PLAN_CACHE::get_hashKey(std::string queryString){
  return queryString;
}

//@Prints out all key-value pairs from plan_cache_dictionary. 
void PLAN_CACHE::print_dict(){
  for(auto pair : plan_cache_dictionary){
    std::cout << "key:"  << pair.first << ". Value: " << pair.second.getAccessPath() << std::endl;
  }
};
