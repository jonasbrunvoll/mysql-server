#ifndef SQL_PLAN_ROOT_INCLUDED
#define SQL_PLAN_ROOT_INCLUDED

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <chrono>
#include <thread>

#include "include/my_alloc.h"           // MEM_ROOT 
#include "include/lex_string.h"         // LEX_STRING
#include "include/sql_string.h"         // String 
#include "sql/sql_tmp_table.h"          // Table

class AccessPath;
class Query_block;

// Save before refactoring of variable name and variables.

struct stmt_param {
    std::string varname;
    std::string val;
    int param_type;
};

class PLAN_ROOT {
    unsigned int timestamp_created;
    unsigned int timestamp_last_used; 
    bool optimized_status = false;
    std::vector <stmt_param> param_set;
    std::map<Query_block*, AccessPath*> access_paths;
    std::vector <TABLE*> temp_table_ptrs; 
    public:
        PLAN_ROOT(std::vector<stmt_param> _param_set) {
            param_set = _param_set;
            set_timestamp_created();
            set_timestamp_last_used();
        }
        MEM_ROOT mem_root;
        bool get_optimized_status();
        void set_optimized_status(bool _status);
        std::vector<stmt_param> get_param_set();
        void set_param_set(std::vector<stmt_param> _param_set);
        bool set_access_path(Query_block* _query_block, AccessPath* _access_path);
        void clear_access_paths();
        void set_timestamp_created();
        void set_timestamp_last_used();
        unsigned int get_timestamp_created();
        unsigned int get_timestamp_last_used();
        void add_ptr_temp_table(TABLE* _ptr_temp_table);
        void cleanup_temp_table_ptrs();
    private:
        unsigned int get_timestamp_current_time();
};
#endif /* SQL_PLAN_ROOT_INCLUDED */