/**
 * @file   assigner.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Mon Dec  9 13:29:46 2013
 * 
 * @brief CAssigner class. It contains symbol table and all the memcached staff.
 * 
 */
#ifndef __ASSIGNER_HPP__
#define __ASSIGNER_HPP__

#include <map>
#include <list>
#include <string>
#include <iostream>
#include <libmemcached-1.0/memcached.h>
#include <cregex.hpp>
#include "myexceptions.hpp"
#include "cstate.hpp"

class CAssigner {
    std::map<std::string, char *> m_symTable;
    memcached_st *m_mcached;
public:
    char *getGlobal(const std::string& var) const;
    char *getLocal(const std::string& var) const;
    char *getLocalPtr(const std::string& var) const;
    explicit CAssigner(const std::string& libmemcachedconfig);
    virtual ~CAssigner();
    void resetTable();
    const char *assignGlobal(const std::string& var, const char *val);
    const char *assignLocal(const std::string& var, const char *val);
    void assign(const assignmentList_t* assignment, const CState* state);
    char* getValue(const std::string& var) const;
    std::string& evaluate(const std::string& src, std::string& result, const CState* state);
    friend std::ostream& operator << (std::ostream& os, const CAssigner& cr);
};

std::ostream& operator << (std::ostream& os, const CAssigner& cr);

#endif // #ifndef __ASSIGNER_HPP__

