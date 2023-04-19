#ifndef SQL_PLAN_CACHE_INCLUDED
#define SQL_PLAN_CACHE_INCLUDED

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include "sql_class.h"                        // THD
#include "sql_plan_root.h"                    // PLAN_ROOT
#include "include/my_alloc.h"                 // MEM_ROOT
#include "include/field_types.h"              // Enum_field_types

class AccessPath;
class Prepared_statment;

typedef std::pair<Prepared_statement*, int> plan_root_key;


// Enum values for entry_logic. 
enum entry_logic {
    UNDEFINED_EL,
    ONE_ENTRY,
    N_ENTRIES
};

// Enum values for match logic.
enum match_logic {
    UNDEFINED_ML,
    EXACT_MATCH,
    INEXACT_MATCH
};


// Enum values for replacment logic.
enum replacement_logic {
    UNDEFINED_RL,
    FIFO,
    LILO,
    LRU,
    WORST_MATCH
};

class PLAN_CACHE {
    unsigned int cache_limit;
    unsigned int version_limit;
    plan_root_key key_active_plan_root;
    std::map<plan_root_key, PLAN_ROOT> plan_roots;
    std::map<std::string, match_logic> match_logics;
    std::map<std::string, entry_logic> entry_logics;
    std::map<std::string, replacement_logic> replacement_logics;

   

 public:
    PLAN_CACHE(){
        cache_limit = 4;
        version_limit = 2;
        entry_logics["ONE_ENTRY"] = ONE_ENTRY; 
        entry_logics["N_ENTRIES"] = N_ENTRIES; 
        match_logics["EXACT_MATCH"] = EXACT_MATCH;    
        match_logics["INEXACT_MATCH"] = INEXACT_MATCH;
        replacement_logics["LRU"] = LRU;
        replacement_logics["FIFO"] = FIFO;
        replacement_logics["LILO"] = LILO;
        replacement_logics["WORST_MATCH"] = WORST_MATCH;
        key_active_plan_root = std::make_pair(nullptr, 0);
    }

    void entry(
        std::string _match_logic, 
        std::string _entry_logic,
        std::string _replacement_logic, 
        Prepared_statement* _ptr_prep_stmt, 
        std::vector<prepared_statement_parameter> _param_set
    );

    void cleanup_plan_root(
        THD* thd, 
        Prepared_statement* _ptr_prep_stmt
    );

    void cleanup_tmp_tables();

    void set_optimized_status_plan_root(
        bool _status
    );

    void set_access_path_plan_root(
        Query_block* _query_block, 
        AccessPath* _access_path
    );

    bool is_executing_prep_stmt();
    bool plan_root_is_optimized();
    void clear_key_active_plan_root();
    PLAN_ROOT* get_ptr_active_plan_root();
    Prepared_statement* get_ptr_prep_stmt();

    
    std::string format_if_varchar_parameter(
        enum_field_types _field_type,
        std::string _parameter
    );
    
    // Writing results to log file. 
    void log_results(
        std::clock_t _duration_opt, 
        std::clock_t _duration_exec, 
        bool prepared_statment, 
        std::string _query_string
    );

private:    
    void global_replacement(
        std::string _replacement_logic, 
        Prepared_statement* _ptr_prep_stmt, 
        std::vector<prepared_statement_parameter> _param_set,
        std::vector<plan_root_key> _versions
    );

    void version_replacement(
        std::string _replacement_logic,
        Prepared_statement* _ptr_prep_stmt, 
        std::vector<prepared_statement_parameter> _param_set,
        std::vector<plan_root_key> _versions
    );

    bool plan_root_exists(
        Prepared_statement* _ptr_prep_stmt
    );

    bool add_plan_root(
        std::vector<prepared_statement_parameter> _param_set
    );

    bool exact_match(
        std::vector <prepared_statement_parameter> _s1, 
        std::vector <prepared_statement_parameter> _s2);


    PLAN_ROOT* get_ptr_plan_root(
        plan_root_key _key
    );

    std::vector<plan_root_key> get_version_keys(
        Prepared_statement* _ptr_prep_stmt
    );

    void erase_plan_root(
        plan_root_key _key_plan_root
    );
};
#endif /* SQL_PLAN_CACHE_INCLUDED */

