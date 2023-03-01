#include <iostream>

#include "sql/sql_plan_root.h"
//#include "include/my_alloc.h" /* MEM_ROOT */
//#include "join_optimizer/access_path.h"


bool PLAN_ROOT::pathIsEmpty() {
    if (this->path == 0) return true;
    return false;
};


