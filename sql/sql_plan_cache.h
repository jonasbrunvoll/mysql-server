#ifndef SQL_PLAN_CACHE_INCLUDED
#define SQL_PLAN_CACHE_INCLUDED

#include <iostream>
#include <map>

#include "sql_plan_cache_item.h"

class AccessPath;

class PLAN_CACHE {
    std::map<std::string, PLAN_CACHE_ITEM> plan_cache_dictionary;
 public:
    PLAN_CACHE(){}
    bool insert_item(std::string key, PLAN_CACHE_ITEM item);
    bool remove_item(std::string key);
    bool is_empty();
    bool contains_item(std::string key);
    std::string get_hashKey(std::string queryString);
    void print_dict();
    //void log_m_root_access_path_type(AccessPath *m_root_access_path);
};
#endif /* SQL_PLAN_CACHE_INCLUDED */
