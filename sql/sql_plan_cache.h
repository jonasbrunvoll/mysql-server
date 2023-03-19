#ifndef SQL_PLAN_CACHE_INCLUDED
#define SQL_PLAN_CACHE_INCLUDED

#include <iostream>
#include <map>
#include <vector>

#include "sql_class.h"                        // THD
#include "include/my_alloc.h"                 // MEM_ROOT
#include "sql_plan_root.h"                    // PLAN_ROOT

class AccessPath;
class Prepared_statment;


class PLAN_CACHE {
    bool executing_prep_stmt = false;
    std::map<Prepared_statement*, PLAN_ROOT> plan_roots;
    Prepared_statement* ptr_prep_stmt = nullptr;
 public:
    PLAN_CACHE(){}
    bool add_plan_root();
    bool plan_root_pair_exists(Prepared_statement* ptr_prep_stmt);
    void set_executing_prep_stmt();
    bool is_executing_prep_stmt();
    bool plan_root_is_optimized();
    void plan_root_set_optimized();
    void set_access_path(Query_block* query_block, AccessPath* access_path);
    void set_ptr_prep_stmt(Prepared_statement* ptr_prep_stmt);
    Prepared_statement* get_ptr_prep_stmt();
    PLAN_ROOT* get_ptr_plan_root();
    void cleanup_plan_root(Prepared_statement* _stmt); 
};
#endif /* SQL_PLAN_CACHE_INCLUDED */
