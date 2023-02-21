
#ifndef SQL_PLAN_CACHE_ITEM_INCLUDED
#define SQL_PLAN_CACHE_ITEM_INCLUDED
//#include "sql/join_optimizer/access_path.h"

#include <iostream>

class AccessPath;

class PLAN_CACHE_ITEM {
    AccessPath *accessPath;
 public:
    PLAN_CACHE_ITEM(AccessPath *m_root_access_path); 
    int getAccessPath();
    //PLAN_CACHE_ITEM(string name) : name(name) {}
    //void log_accessPath_type();
    void log_m_root_access_path_type(AccessPath *m_root_access_path);
};
#endif /* SQL_PLAN_CACHE_ITEM_INCLUDED */
