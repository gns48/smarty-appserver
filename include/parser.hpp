/**
 * @file   parser.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Mon Nov 11 22:57:14 2013
 * 
 * @brief  parser definitions
 */

#ifndef __PARSER_HPP__
#define __PARSER_HPP__

#include <string>
#include <map>
#include <cstring>
#include <boost/property_tree/ptree.hpp>
#include "myexceptions.hpp"
#include "cstate.hpp"
#include "cregex.hpp"

typedef std::map<int, CState*> stateMap_t;
typedef std::map<std::string, stateMap_t*> scriptMap_t;
typedef std::map<std::string, int> entryMap_t;

stateMap_t* parseScript(const std::string& file);
bool findConfigFile(std::string& cpath, const char* what);
int openRegexCollection(const char *path = nullptr);
void freeRegexCollection();

bool is_matched(const char *name,
                const std::string& line,
                regmatch_t *regmatch,
                int flags,
                const std::string& file,
                unsigned lineCounter);

bool is_variable(const std::string& line, size_t *so, size_t *eo, const char *vname);

assignmentList_t* parseAssignment(const std::string& line,
                                  const int statenum,
                                  const std::string& file,
                                  unsigned counter);

#endif // #ifndef __PARSER_HPP__


