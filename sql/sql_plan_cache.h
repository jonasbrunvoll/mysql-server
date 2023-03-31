#ifndef SQL_PLAN_CACHE_INCLUDED
#define SQL_PLAN_CACHE_INCLUDED

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include "sql_class.h"                        // THD
#include "include/my_alloc.h"                 // MEM_ROOT
#include "sql_plan_root.h"                    // PLAN_ROOT

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
    INEXACT_MATCH,
    WORST_MATCH 
};


// Enum values for replacment logic.
enum replacement_logic {
    UNDEFINED_RL,
    FIFO,
    LILO,
    LRU
};

class PLAN_CACHE {
    int cache_limit;
    int version_limit;
    plan_root_key key_active_plan_root;
    std::map<plan_root_key, PLAN_ROOT> plan_roots;
    std::map<std::string, match_logic> match_logics;
    std::map<std::string, entry_logic> entry_logics;
    std::map<std::string, replacement_logic> replacement_logics;

   

 public:
    PLAN_CACHE(){
        version_limit = 4;
        key_active_plan_root = std::make_pair(nullptr, 0);
        match_logics["EXACT_MATCH"] = EXACT_MATCH;    
        match_logics["INEXACT_MATCH"] = INEXACT_MATCH;
        match_logics["WORST_MATCH"] = WORST_MATCH; 
        entry_logics["ONE_ENTRY"] = ONE_ENTRY; 
        entry_logics["N_ENTRIES"] = N_ENTRIES; 
        replacement_logics["FIFO"] = FIFO;
        replacement_logics["LILO"] = LILO;
        replacement_logics["LRU"] = LRU;
    }
    void entry(std::string _match_logic, 
               std::string _entry_logic,
               std::string _replacement_logic, 
               Prepared_statement* _ptr_prep_stmt, 
               std::vector<stmt_param> _param_set);
    void clear_key_active_plan_root();
    void cleanup_plan_root(THD* thd, Prepared_statement* _ptr_prep_stmt); 
    void set_access_path_plan_root(Query_block* _query_block, AccessPath* _access_path);
    bool plan_root_is_optimized();
    void set_optimized_status_plan_root(bool _status);


    bool is_executing_prep_stmt();
    
    PLAN_ROOT* get_ptr_active_plan_root();
    Prepared_statement* get_ptr_prep_stmt();

     // Pointer to match function used to comapre param sets. 
    typedef bool (PLAN_CACHE::*MatchFunction)(
        std::vector<stmt_param>,
        std::vector<stmt_param>);
    
    // Unused functions:
    plan_root_key get_key_active_plan_root();


private:
    bool plan_root_exists(Prepared_statement* _ptr_prep_stmt);
    bool add_plan_root(std::vector<stmt_param> _param_set);
    bool exact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2);
    bool inexact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2);
    int num_plan_root_entries(Prepared_statement* _ptr_prep_stmt);
};
#endif /* SQL_PLAN_CACHE_INCLUDED */

