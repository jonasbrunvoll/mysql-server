#ifndef SQL_PLAN_ROOT_INCLUDED
#define SQL_PLAN_ROOT_INCLUDED

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <list>


#include "include/my_alloc.h"           // MEM_ROOT 
#include "include/lex_string.h"         // LEX_STRING
#include "include/sql_string.h"         // String 

class AccessPath;
class Query_block;

struct stmt_param {
    std::string varname;
    std::string val;
    int param_type;
};

class PLAN_ROOT {
    bool optimized_status = false;
    std::vector <stmt_param> param_set;
    std::map<Query_block*, AccessPath*> access_paths;
    public:
        PLAN_ROOT(std::vector<stmt_param> _param_set) {
            param_set = _param_set;
        }
        // TODO: Make deconstructor work.
        MEM_ROOT mem_root;
        bool get_optimized_status();
        void set_optimized_status(bool _status);
        std::vector<stmt_param> get_param_set();
        void set_param_set(std::vector<stmt_param> _param_set);
        bool add_access_path(Query_block* _query_block, AccessPath* _access_path);
        void clear_access_paths();
};

#endif /* SQL_PLAN_ROOT_INCLUDED */