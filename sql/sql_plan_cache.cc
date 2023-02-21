#include <iostream>
#include <stdio.h>
#include <string.h>

#include "sql/sql_plan_cache.h"
#include "sql/sql_plan_cache_item.h"


class AccessPath;

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


/*
void PLAN_CACHE::log_m_root_access_path_type(AccessPath *m_root_access_path){
  printf("\nLogg m_root_access_path: %d\n", m_root_access_path->type);
};



void PLAN_CACHE::add_access_path(string key, string name){
  //PLAN_CACHE_ITEM item(name);
  this.plan_cache_dictionary.insert(pair<string, string>(key, name)); 
}

void PLAN_CACHE::show_key_value_pairs(){
  for(auto pair: plan_cache_dictionary)
    printf("Key: %s, value: %s\n", pair.first, pair.second);
}
*/

//void PLAN_CACHE::remove_access_path(string key, AccessPath *m_root


/*
Constructor:

Initializes plan_cache_dictionary. Key is a char value and value is a m_root_access_path instance.
PLAN_CACHE::PLAN_CACHE(char key, PLAN_CACHE_ITEM::plan_cache_item){
  this.plan_cache_dictionary  = {key : value}; 
}

void PLAN_CACHE::log_hello_world(){
  printf("\nHello World from sql_plan_cache.cc\n");
};

void PLAN_CACHE::log_m_root_access_path_type(AccessPath *m_root_access_path){
  printf("\nLogg m_root_access_path: %d\n", print_m_root_access_path->type);
};

void PLAN_CACHE::print_plan_cache_items(){
  printf(plan_cache_dictionary);
}

void PLAN_CACHE::get_plan_cache_item(char key){
  printf(plan_cache_dictionary[key]);
}
*/ 