/**
 * @file   assigner.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 00:19:27 2013
 * 
 * @brief  CAssigner class implementation
 * 
 */

#include "config.h"
#include <sys/types.h>
#include <cstring>
#include <boost/lexical_cast.hpp>
#include "myexceptions.hpp"
#include "cassigner.hpp"
#include "parser.hpp"

CAssigner::CAssigner(const std::string& libmemcachedconfig) {
    m_mcached = memcached(libmemcachedconfig.c_str(), libmemcachedconfig.length());
    if(!m_mcached) throw std::runtime_error(memcached_last_error_message(NULL));
}

CAssigner::~CAssigner() {
    resetTable();
    if(m_mcached) memcached_free(m_mcached);
}

const char* CAssigner::assignGlobal(const std::string& var, const char *val) {
    if(val) {
        memcached_return_t rv = memcached_set(m_mcached, var.c_str(), var.length(),
                                              val, strlen(val)+1, 0, 0);
        if(rv != MEMCACHED_SUCCESS)
            throw memcache_error(memcached_strerror(m_mcached, rv), rv);
        free(const_cast<char*>(val));        
    }
    return 0;
}

const char* CAssigner::assignLocal(const std::string& var, const char *val) {
    if(val) {
        const auto it = m_symTable.find(var);
        if(it != m_symTable.end()) free(it->second);
        m_symTable[var] = const_cast<char*>(val);
    }
    return val;
}

char* CAssigner::getGlobal(const std::string& var) const {
    size_t outlen;
    uint32_t flags;
    memcached_return_t rv;
    char* out = memcached_get(m_mcached, var.c_str(), var.length(), &outlen, &flags, &rv);
    if(rv != MEMCACHED_SUCCESS) throw memcache_error(memcached_strerror(m_mcached, rv), rv);
    return out;
}

char* CAssigner::getLocal(const std::string& var) const {
    const auto &it = m_symTable.find(var);
    // strdup to conform to the libmemcached return policy
    if(it != m_symTable.end()) return strdup(it->second); 
    return  nullptr;
}

// like getLocal but do not allocate memory, return just pointer
char* CAssigner::getLocalPtr(const std::string& var) const {
    const auto &it = m_symTable.find(var);
    if(it != m_symTable.end()) return it->second; 
    return  nullptr;
}

void CAssigner::resetTable() {
    for(const auto &it : m_symTable) free(it.second);    
    m_symTable.clear();
}

void CAssigner::assign(const assignmentList_t* assignment, const CState* state) {
    char *val = 0;
    assignmentList_t::const_iterator it = assignment->begin();
    std::string str_asmnt;
    
    const std::string& var = *it++;
    while(it != assignment->end()) {
        switch((*it)[0]) {
        case '@':
            val = getLocal(*it);
            break;
        case '&':
            val = getGlobal(*it);
            break;
        case '$':
            val = state->getPropositional((*it));
            break;
        case '"':
            // string assignment, evaluate variables
            evaluate((*it).substr(1), str_asmnt, state);
            val = strdup(str_asmnt.c_str());
            break;
        case '\'':
            //  string assignment, assign "as is"
            val = strdup((*it).substr(1).c_str());
            break;
        }
        if(val) break;
        ++it;
    }
    if(val) {
        if(var[0] == '&') assignGlobal(var, val);
        else assignLocal(var, val);
    }
}

char* CAssigner::getValue(const std::string& var) const {
    return var[0] == '&' ? getGlobal(var) : getLocal(var);
}

std::string& CAssigner::evaluate(const std::string& src, std::string& result, const CState* state) {
    std::string::size_type i;
    size_t so, eo; // offsets
    result = "";
    i = 0;
    while(i < src.length()) {
        if(src[i] == '\\') {
            // escaped @ or & for example, skip backslash, copy symbol to output string
            i++;
        }
        else if(src[i] == '@' || src[i] == '&' || src[i] == '$') {
            char *val = nullptr;
            bool matched = false;
            // begining of variable, evaluate
            if(is_variable(src.substr(i), &so, &eo, "lvar")) {
                // local variable
                val = getValue(src.substr(i, eo - so));
                matched = true;
            }
            else if(is_variable(src.substr(i), &so, &eo, "slvar")) {
                // local variable in short form
                std::string var("@");
                var.append(boost::lexical_cast<std::string>(state->get_number()));
                var.push_back('.');
                var.append(src.substr(i+1, eo - so - 1)); // skip '@' in substring
                val = getValue(var);
                matched = true;
            }
            else if(is_variable(src.substr(i), &so, &eo, "gvar")) {
                // global variable
                val = getValue(src.substr(i, eo - so));
                matched = true;
            }
            else if(is_variable(src.substr(i), &so, &eo, "pvar")) {
                // propositional variable
                val = state->getPropositional(src.substr(i, eo - so));
                matched = true;
            }
            if(matched) {
                if(val) {
                    // adding variable's value: result = "... some_val"
                    result.append(val);
                    free(val);
                }
                // advance index to the length of variable name
                i += (eo - so);
                continue;
            }
        }
        result.push_back(src[i++]);
    }
    return result;
}

std::ostream& operator << (std::ostream& os, const CAssigner& cr) {
    for(const auto &it : cr.m_symTable)
        os << it.first << " = " << it.second << std::endl;
    return os;
}





