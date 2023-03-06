#ifndef SQL_PLAN_CACHE_INCLUDED
#define SQL_PLAN_CACHE_INCLUDED

#include <iostream>
#include <map>

#include "sql_class.h"                        // THD
#include "include/my_alloc.h"   
#include "sql_plan_root.h"

class AccessPath;

class PLAN_CACHE {
    bool executing_prep_stmt = false;
    std::map<std::string, PLAN_ROOT> plan_roots;
 public:
    PLAN_CACHE(){}
    bool add_plan_root(std::string hash_key);
    bool swap_mem_root(THD* thd, std::string hash_key);
    bool is_empty();
    bool plan_root_exists(std::string hash_key);
    std::string create_hash_key(std::string query);
    void set_executing_prep_stmt();
    bool is_executing_prep_stmt();
    void set_access_path(std::string hash_key, AccessPath* access_path);
};
#endif /* SQL_PLAN_CACHE_INCLUDED */
