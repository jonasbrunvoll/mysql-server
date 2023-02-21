#include <iostream>
#include <string>
#include "sql/sql_plan_cache_item.h"

#include "join_optimizer/access_path.h"


PLAN_CACHE_ITEM::PLAN_CACHE_ITEM(AccessPath *m_root_access_path){
  this->accessPath = m_root_access_path;
};


int PLAN_CACHE_ITEM::getAccessPath(){
  return accessPath->type;
};


void PLAN_CACHE_ITEM::log_m_root_access_path_type(AccessPath *m_root_access_path){
  printf("\nLogg m_root_access_path: %d\n", m_root_access_path->type);
};


/*
void PLAN_CACHE_ITEM::log_accessPath_type(){
  printf("\nType number: %s\n", name);
};
*/
