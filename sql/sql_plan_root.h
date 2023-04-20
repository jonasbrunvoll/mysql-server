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

struct prepared_statement_parameter {
    std::string varname;
    std::string val;
    int param_type;
};

class PLAN_ROOT {
    // Attributes  
    bool optimized = false;
    std::map<Query_block*, AccessPath*> access_path_pointers;
    std::vector <prepared_statement_parameter> parameters;
    std::vector <TABLE*> temp_table_pointers; 
    unsigned int timestamp_created;
    unsigned int timestamp_last_accessed; 
    
    public:
        PLAN_ROOT(std::vector<prepared_statement_parameter> _parameters) {
            parameters = _parameters;
            set_timestamp_created();
            set_timestamp_last_accessed();
        }
        
        MEM_ROOT mem_root;
 
        void set_optimized_status(bool _optimized_status);
        bool is_optimized();
        
        bool add_access_path(Query_block* _query_block, AccessPath* _access_path);
        void clear_access_paths();
        
        void set_parameters(std::vector<prepared_statement_parameter> _parameters);
        std::vector<prepared_statement_parameter> get_parameters();
        
        void append_temp_table(TABLE* _temp_table);
        void free_temp_tables();
        
        void set_timestamp_created();
        void set_timestamp_last_accessed();
        unsigned int get_timestamp_created();
        unsigned int get_timestamp_last_accessed();
    
    private:
        unsigned int get_current_timestamp();
};
#endif /* SQL_PLAN_ROOT_INCLUDED */