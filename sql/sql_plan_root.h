#ifndef SQL_PLAN_ROOT_INCLUDED
#define SQL_PLAN_ROOT_INCLUDED

#include <iostream>

#include "include/my_alloc.h" /* MEM_ROOT */

class AccessPath;

class PLAN_ROOT {
    public:
        AccessPath* path = nullptr;
        MEM_ROOT mem_root;
        PLAN_ROOT(){}
};

#endif /* SQL_PLAN_ROOT_INCLUDED */