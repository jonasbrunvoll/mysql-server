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

class AccessPath;
class Query_block;

struct stmt_param {
    std::string varname;
    std::string val;
    int param_type;
};

class PLAN_ROOT {
    /*
        Counter to keep track of the number of entries 
        since this plan root have been run.
    */
    int entries_counter;
    unsigned int timestamp_created;
    unsigned int timestamp_last_used; 
    bool optimized_status = false;
    std::vector <stmt_param> param_set;
    std::map<Query_block*, AccessPath*> access_paths;
    public:
        PLAN_ROOT(std::vector<stmt_param> _param_set) {
            entries_counter = 0;
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
        void increment_entries();
        void set_entries(int _entries);
        int get_entries();
        void set_timestamp_created();
        void set_timestamp_last_used();
        unsigned int get_timestamp_created();
        unsigned int get_timestamp_last_used();
    private:
        unsigned int get_timestamp_current_time();
};
#endif /* SQL_PLAN_ROOT_INCLUDED */