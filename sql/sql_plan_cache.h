#ifndef SQL_PLAN_CACHE_INCLUDED
#define SQL_PLAN_CACHE_INCLUDED

#include <iostream>
#include <map>

#include "sql_plan_cache_item.h"
//#include "include/my_alloc.h"

//class AccessPath;


class PLAN_CACHE {
    std::map<std::string, PLAN_CACHE_ITEM> plan_cache_dictionary;
    //std::map<std::string, *MEM_ROOT> test_dict;
 public:
    PLAN_CACHE(){}
    //bool insert_test_dict(std::string key, AccessPath path);
    bool insert_item(std::string key, PLAN_CACHE_ITEM item);
    bool remove_item(std::string key);
    bool is_empty();
    bool contains_item(std::string key);
    std::string get_hashKey(std::string queryString);
    void print_dict();
};
#endif /* SQL_PLAN_CACHE_INCLUDED */
