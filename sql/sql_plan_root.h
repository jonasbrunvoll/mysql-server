#ifndef SQL_PLAN_ROOT_INCLUDED
#define SQL_PLAN_ROOT_INCLUDED

#include <iostream>
#include <map>


//#include "sql_class.h"              // THD
#include "include/my_alloc.h"       // MEM_ROOT 
//#include "sql/sql_lex.h"            // Query_block

class AccessPath;

class PLAN_ROOT {
    public:
        AccessPath* path = nullptr;
        MEM_ROOT mem_root;
        bool is_optimized = false;
        std::map<std::string, AccessPath*> access_paths;
        PLAN_ROOT(){}

};

#endif /* SQL_PLAN_ROOT_INCLUDED */