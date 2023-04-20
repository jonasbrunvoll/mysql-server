#include <iostream>
#include <vector> 
#include <list>

#include "sql/sql_plan_root.h"
#include "sql/join_optimizer/access_path.h"   // AccessPath
#include "sql/sql_lex.h"                      // Query_block
 

void PLAN_ROOT::set_optimized_status(bool _optimized_status){
    optimized = _optimized_status;
};


bool PLAN_ROOT::is_optimized(){
    return optimized;
};

bool PLAN_ROOT::add_access_path(Query_block* _query_block, AccessPath* _access_path){
    auto it = access_path_pointers.emplace(_query_block, _access_path);
    if (!it.second) return true;
    return false;
};

void PLAN_ROOT::clear_access_paths(){
    access_path_pointers.clear();
};

void PLAN_ROOT::append_temp_table(TABLE* _temp_table){
    temp_table_pointers.push_back(_temp_table);
};

void PLAN_ROOT::free_temp_tables(){
    for (auto &temp_table: temp_table_pointers){
        if (--temp_table->s->tmp_open_count > 0) {
            temp_table->file->ha_close();
        } else {
            temp_table->file->ha_drop_table(temp_table->s->table_name.str);
            temp_table->set_deleted();   
        } 
    }
    temp_table_pointers.clear();
};

void PLAN_ROOT::set_parameters(std::vector<prepared_statement_parameter> _parameters){
    parameters = _parameters;
};

std::vector<prepared_statement_parameter> PLAN_ROOT::get_parameters(){
    return parameters;
};

void PLAN_ROOT::set_timestamp_created(){
    timestamp_created = get_current_timestamp();
}

void PLAN_ROOT::set_timestamp_last_accessed(){
    timestamp_last_accessed = get_current_timestamp();
}

unsigned int PLAN_ROOT::get_timestamp_created(){
    return timestamp_created;
};

unsigned int PLAN_ROOT::get_timestamp_last_accessed(){
    return timestamp_last_accessed;
};

unsigned int PLAN_ROOT::get_current_timestamp(){
    const auto p = std::chrono::system_clock::now();
    return (unsigned int) std::chrono::duration_cast<std::chrono::seconds>(p.time_since_epoch()).count();
}

