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

typedef std::pair<Prepared_statement*, int> key_pair;

// Enum values for cache versions. 
enum cache_version { 
    UNDEFINED,
    EXACT_MATCH_1, 
    INEXACT_MATCH_1,
    EXACT_MATCH_N, 
    INEXACT_MATCH_N
};

class PLAN_CACHE {
    // Map to associate the strings with the enum cache version values
    std::map<std::string, cache_version> cache_version_whitelist;
    Prepared_statement* ptr_prep_stmt = nullptr;
    std::map<Prepared_statement*, PLAN_ROOT> plan_roots;
    std::map<key_pair, PLAN_ROOT> plan_roots2;
 public:
    PLAN_CACHE(){
        cache_version_whitelist["EXACT_MATCH_1"] = EXACT_MATCH_1;
        cache_version_whitelist["INEXACT_MATCH_1"] = INEXACT_MATCH_1;
        cache_version_whitelist["EXACT_MATCH_N"] = EXACT_MATCH_N;
        cache_version_whitelist["INEXACT_MATCH_N"] = INEXACT_MATCH_N;
    }
    void entry(std::string _cache_version, 
               Prepared_statement* _ptr_prep_stmt, 
               std::vector<stmt_param> _param_set);
    bool add_plan_root(std::vector<stmt_param> _param_set);
    bool is_executing_prep_stmt();
    bool plan_root_is_optimized();
    bool plan_root_pair_exists(Prepared_statement* _ptr_prep_stmt);
    void set_optimized_status_plan_root(bool _status);
    void cleanup_plan_root(Prepared_statement* _ptr_prep_stmt); 
    void set_ptr_prep_stmt(Prepared_statement* _ptr_prep_stmt);
    void set_access_path(Query_block* _query_block, AccessPath* _access_path);
    PLAN_ROOT* get_ptr_plan_root();
    Prepared_statement* get_ptr_prep_stmt();
    bool add_plan_root2(Prepared_statement* _ptr_prep_stmt);
    int  num_versions_plan_root2(Prepared_statement* _ptr_prep_stmt);
    bool plan_root_pair_exists2(Prepared_statement* _ptr_prep_stmt);

private:
    bool exact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2);
    bool inexact_match(std::vector <stmt_param> _s1, std::vector <stmt_param> _s2);
};
#endif /* SQL_PLAN_CACHE_INCLUDED */

