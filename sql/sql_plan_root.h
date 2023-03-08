#ifndef SQL_PLAN_ROOT_INCLUDED
#define SQL_PLAN_ROOT_INCLUDED

#include <iostream>
#include <map>


//#include "sql_class.h"              // THD
#include "include/my_alloc.h"       // MEM_ROOT 
//#include "sql/sql_lex.h"            // Query_block

class AccessPath;
class Query_block;

class PLAN_ROOT {
    std::map<Query_block*, AccessPath*> access_paths;
    public:
        PLAN_ROOT(){}
        bool add_access_path(Query_block* query_block, AccessPath* access_path);
        bool access_path_exists(Query_block* ptr_query);
        AccessPath* path = nullptr;
        MEM_ROOT mem_root;
        bool is_optimized = false;

};

#endif /* SQL_PLAN_ROOT_INCLUDED */