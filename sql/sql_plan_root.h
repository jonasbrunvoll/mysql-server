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
    std::list<std::vector <stmt_param>> param_sets;
    std::map<Query_block*, AccessPath*> access_paths;
    public:
        PLAN_ROOT(){}
        // TODO: Make deconstructor work.
        MEM_ROOT mem_root;
        bool get_optimized_status();
        void set_optimized_status(bool _status);
        bool add_access_path(Query_block* _query_block, AccessPath* _access_path);
        //bool access_path_exists(Query_block* _ptr_query);
        void add_param_set(std::vector<stmt_param> _param_set);
        bool compare_param_sets(std::vector <stmt_param> _s1,std::vector <stmt_param> _s2);

};

#endif /* SQL_PLAN_ROOT_INCLUDED */