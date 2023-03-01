#ifndef SQL_PLAN_ROOT_INCLUDED
#define SQL_PLAN_ROOT_INCLUDED

#include <iostream>

#include "include/my_alloc.h" /* MEM_ROOT */

class AccessPath;

class PLAN_ROOT {
    public:
    AccessPath* path = nullptr;
    MEM_ROOT plan_mem_root;
    PLAN_ROOT(){}
    bool pathIsEmpty();
    //bool set_plan_mem_root(AccessPath *m_root_access_path);

};

#endif /* SQL_PLAN_ROOT_INCLUDED */