#ifndef SQL_PLAN_CACHE_INCLUDED
#define SQL_PLAN_CACHE_INCLUDED

#include <iostream>
#include <map>
#include <vector>
#include <utility>

#include "sql_class.h"                        // THD
#include "include/my_alloc.h"                 // MEM_ROOT
#include "sql_plan_root.h"                    // PLAN_ROOT

class AccessPath;
class Prepared_statment;

typedef std::pair<Prepared_statement*, int> plan_root_key;

// Enum values for cache versions. 
enum cache_version { 
    UNDEFINED,
    EXACT_MATCH_1, 
    INEXACT_MATCH_1,
    EXACT_MATCH_N, 
    INEXACT_MATCH_N
};

// Enum values for replacment logic.
enum replacement_logic {
    UNDEFINDED,
    FIFO,
    LILO
};

class PLAN_CACHE {
    int max_num_entries;
    plan_root_key key_active_plan_root;
    std::map<plan_root_key, PLAN_ROOT> plan_roots;
    std::map<std::string, cache_version> cache_versions;
    std::map<std::string, replacement_logic> replacement_logics;
 public:
    PLAN_CACHE(){
        max_num_entries = 4;
        key_active_plan_root =  std::make_pair(nullptr, 0);
        cache_versions["EXACT_MATCH_1"] = EXACT_MATCH_1;
        cache_versions["EXACT_MATCH_N"] = EXACT_MATCH_N;
        cache_versions["INEXACT_MATCH_1"] = INEXACT_MATCH_1;
        cache_versions["INEXACT_MATCH_N"] = INEXACT_MATCH_N;
        replacement_logics["FIFO"] = FIFO;
        replacement_logics["LILO"] = LILO;
    }
    void entry(std::string _cache_version,
               std::string _replacement_logic, 
               Prepared_statement* _ptr_prep_stmt, 
               std::vector<stmt_param> _param_set);
    void clear_variabels();
    void cleanup_plan_root(THD* thd, Prepared_statement* _ptr_prep_stmt); 
    void set_access_path(Query_block* _query_block, AccessPath* _access_path);
    bool plan_root_is_optimized();
    void set_optimized_status_plan_root(bool _status);


    bool is_executing_prep_stmt();
    
    PLAN_ROOT* get_ptr_active_plan_root();
    Prepared_statement* get_ptr_prep_stmt();
    
    // Unused functions:
    plan_root_key get_key_active_plan_root();


private:
    void set_key_active_plan_root(plan_root_key _key);
    bool plan_root_exists(Prepared_statement* _ptr_prep_stmt);
    bool add_plan_root(std::vector<stmt_param> _param_set);
    bool exact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2);
    bool inexact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2);
    int num_plan_root_entries(Prepared_statement* _ptr_prep_stmt);
};
#endif /* SQL_PLAN_CACHE_INCLUDED */

