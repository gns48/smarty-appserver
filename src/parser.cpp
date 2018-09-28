/**
 * @file   parser.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Mon Nov 11 15:03:30 2013
 * @brief  State language parser
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <regex.h>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include "apputils.hpp"
#include "cregex.hpp"
#include "parser.hpp"

namespace fs = boost::filesystem;

extern const char *nested_state;
extern const char *syntax_error;
extern const char *invalid_state;
extern const char *duplicate_state;
extern const char *unfinished_state;
extern const char *assmnt_error;
extern const char *rexfileName;      // regex library

static CRegexCollection *rexCollection = nullptr;      // result of rexfileName processing

/* different parsing utilities */

/**
 * @fn bool findConfigFile(std::string& cpath, const char* what)
 * @brief function tries to find a file in one of predefined directories.
 * @param cpath (out) canonical path (if file found, else -- unchanged)
 * @param what (in) file name to find in predefined directories
 * @return true if file exists ant it is a regular file, else -- false
 */
bool findConfigFile(std::string& cpath, const char* what) {
    // configuration file search directories.
    extern const char *configDirs[]; // defined in templates.cpp.in

    for(int i = 0; configDirs[i] != 0; i++) {
        fs::path p (configDirs[i]);
        p += what;
        if(fs::exists(p) && fs::is_regular_file(p)) {
            cpath = fs::canonical(p).string();
            return true;
        }
    }
    return false;
}

/**
 * @fn int openRegexCollection()
 * @brief function fills the global regex collection from an external file
 * @param none 
 * @return 0 if success, else EINVAL if file can not be parsed of ENOENT if file not found
 */
int openRegexCollection(const char* path) {
    std::string rexlibPath;
    if(path) rexlibPath = path;
    else if(!findConfigFile(rexlibPath, rexfileName)) {
        std::cerr << "Can not find regex library" << std::endl;
        return ENOENT;
    }

    try {
        // global, see above, this module
        rexCollection = new CRegexCollection(rexlibPath.c_str());
    }
    catch(...) {
        std::cerr << "Can not parse " << rexlibPath << std::endl;
        return EINVAL;
    }
    
    return 0;
}

void freeRegexCollection() {
    delete rexCollection;
}

/**
 * @fn bool is_matched(const char *name,
 *               const std::string& line,
 *               regmatch_t *regmatch,
 *               int flags,
 *               const std::string& file,
 *               unsigned lineCounter)
 * 
 * @brief function tries to match regex passed by it's name in collection
 * @param const char *name -- (in) regex name
 * @param const std::string& line -- (in) string to match
 * @param regmatch_t *regmatch -- (in) array of matched subexpressions offsets
 * @param int flags -- (in) flags for rexexec, see regex(3)
 * @param const std::string& file -- (in) file name of file containing matching string
 * @param unsigned lineCounter -- (in) matching string line number (for debug output)
 * @return true if regex matched, else -- false
 */
bool is_matched(const char *name,
                const std::string& line,
                regmatch_t *regmatch,
                int flags,
                const std::string& file,
                unsigned lineCounter)
{
    int rv = regexec((*rexCollection)[name]->get(),
                     line.c_str(), regmatch ? REGMATCH_COUNT : 0, regmatch, flags);
    if(rv && rv != REG_NOMATCH) {
        throw parser_error(file, (*rexCollection)[name]->getError(rv), lineCounter);
    }
#ifdef DEBUG    
    std::cout << __func__ << ":" << file << ":" << lineCounter << ": " 
              << name << ": " << line << ", rv = " << rv << std::endl;
#endif
    return rv == 0;
}

/**
 * @fn is_variable(const std::string& line, size_t *so, size_t *eo, const char *vname)
 * @brief function tries to match a variable name by regex in the line passed  
 * @param const std::string& line -- (in) string to match
 * @param size_t *so -- (out) variable's name start offset
 * @param size_t *eo -- (out) variable's name end offset
 * @param const char *vname -- (in) name of regex in the collection that matches a
 * variable name
 * @return true if regex matched, else -- false
 */
bool is_variable(const std::string& line, size_t *so, size_t *eo, const char *vname) {
    regmatch_t regmatch[REGMATCH_COUNT];
    int rv = regexec((*rexCollection)[vname]->get(), line.c_str(), REGMATCH_COUNT, regmatch, 0);
    if(!rv) {
        *so = regmatch[1].rm_so;
        *eo = regmatch[1].rm_eo;
        return true;
    }
    else if(rv != REG_NOMATCH) throw std::runtime_error((*rexCollection)["lvar"]->getError(rv));
    return false;
}

static inline int fetch_int(std::string& line, int fnum, regmatch_t *regmatch) {
    regoff_t len = regmatch[fnum].rm_eo - regmatch[fnum].rm_so;
#ifdef DEBUG
    std::cout << __func__ << ": " << line << ", field " << fnum << ": "
              << line.substr(regmatch[fnum].rm_so, len) << std::endl;
#endif 
    return boost::lexical_cast<int>(line.substr(regmatch[fnum].rm_so, len));
}

static inline std::string prependState(std::string& ref, int statenum) {
    std::string var = "@";
    var.append(boost::lexical_cast<std::string>(statenum));
    var.append(".");
    return var + ref.substr(1);
}

assignmentList_t* parseAssignment(const std::string& line,
                                  const int statenum,
                                  const std::string& file,
                                  unsigned counter)
{
    regmatch_t regmatch[REGMATCH_COUNT];
    assignmentList_t* newlist = nullptr;
    std::string varname;
    bool matched = false;
    bool s_matched = false;
    bool d_matched = false;
    
    // (lvalue)( = )(@|&|$|[A-Za-z])
    if(is_matched("local_equal",   line, regmatch, 0, file, counter) ||
       is_matched("global_equal",  line, regmatch, 0, file, counter))
    {
        varname = line.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
        matched = true;
    }
    else if(is_matched("short_equal", line, regmatch, 0, file, counter)) {
        varname = line.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
        varname = prependState(varname, statenum);
        matched = true;
    }
    else if(is_matched("local_streqs",   line, regmatch, 0, file, counter) ||
            is_matched("global_streqs",  line, regmatch, 0, file, counter))
    {
        varname = line.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
        s_matched = true;
    }
    else if(is_matched("short_streqs", line, regmatch, 0, file, counter)) {
        varname = line.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
        varname = prependState(varname, statenum);
        s_matched = true;
    }
    else if(is_matched("local_streqd",   line, regmatch, 0, file, counter) ||
            is_matched("global_streqd",  line, regmatch, 0, file, counter))
    {
        varname = line.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
        d_matched = true;
    }
    else if(is_matched("short_streqd", line, regmatch, 0, file, counter)) {
        varname = line.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
        varname = prependState(varname, statenum);
        d_matched = true;
    }
    
    if(matched) {
        newlist = new assignmentList_t;
        newlist->push_back(varname);
        // beginning of the "something" -- third group
        std::string subs = line.substr(regmatch[2].rm_so);
        std::string& subsref = subs;
        // parse the rest of string
        while(subsref.length()) {
               // '?'? @110.local 
            if(is_matched("def_local_var", subsref, regmatch, 0, file, counter) ||
               // '?'? &GlobalVar 
               is_matched("def_global_var", subsref, regmatch, 0, file, counter) ||
               // '?'? $3
               is_matched("def_prop", subsref, regmatch, 0, file, counter) ||
               // '?'? $level0.a (json or xml variable in dotted form)
               is_matched("def_json", subsref, regmatch, 0, file, counter))
            {
                varname = subsref.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
            }
            //  local (in short form, expand it)
            else if(is_matched("def_local_short", subsref, regmatch, 0, file, counter)) {
                varname = subsref.substr(regmatch[1].rm_so, regmatch[1].rm_eo - regmatch[1].rm_so);
                varname = prependState(varname, statenum);
            }
            else throw parser_error(file, assmnt_error, counter);
            // variable parsed
            newlist->push_back(varname);
            subsref = subsref.substr(regmatch[1].rm_eo);
        }
    }
    else if(s_matched) {
        // a string in single quotas is assigned
        newlist = new assignmentList_t;
        newlist->push_back(varname);
        std::string strval("'");
        strval.append(line.substr(regmatch[2].rm_so, regmatch[2].rm_eo - regmatch[2].rm_so));
        newlist->push_back(strval);
    }
    else if(d_matched) {
        // a string in double quotas is assigned
        newlist = new assignmentList_t;
        newlist->push_back(varname);
        std::string strval("\"");
        strval.append(line.substr(regmatch[2].rm_so, regmatch[2].rm_eo - regmatch[2].rm_so));
        newlist->push_back(strval);
    }
    else throw parser_error(file, syntax_error, counter);
    return newlist;
}

// used inside the parseScript function.
#define IF_STATE(state_regex, StateClass)                               \
    if(is_matched(state_regex, line, regmatch, 0, file, counter)) { \
        if(in_stateblock) throw parser_error(file, nested_state, counter); \
        else in_stateblock = true, st = new StateClass(stateno = fetch_int(line, 1, regmatch), file); \
        continue;                                                       \
    }

stateMap_t* parseScript(const std::string& file) {
    std::ifstream ifs(file.c_str());
    if(!ifs.is_open()) throw(parser_error(file, "file not found"));

    stateMap_t *sm = new stateMap_t;
    unsigned counter = 0;
    std::string line;
    bool in_stateblock = false;
    regmatch_t regmatch[REGMATCH_COUNT];
    CState *st = nullptr;
    int stateno;
    
    while(getline(ifs, line)) {
        counter++;
        // truncate comments (sharp or semicolon)
        std::string::size_type sp = line.find('#');
        if(sp == std::string::npos) sp = line.find(';');
        if(sp != std::string::npos) { // # found
            std::string::size_type ldq = line.find('\x22');  // double quote
            std::string::size_type rdq = line.rfind('\x22'); // double quote
            std::string::size_type lsq = line.find('\x27');  // single quote
            std::string::size_type rsq = line.rfind('\x27'); // single quote
            if((ldq != std::string::npos && sp < ldq) || // # "...
               (rdq != std::string::npos && sp > rdq) || // ..." #
               (lsq != std::string::npos && sp < lsq) || // # '...
               (rsq != std::string::npos && sp > rsq) || // ...' #
               (ldq == std::string::npos && rdq == std::string::npos &&
                lsq == std::string::npos && rsq == std::string::npos)) // single #
                line.resize(sp); // truncate it
        }
        // skip empty line
        if(is_matched("empty", line, 0, 0, file, counter)) continue;
        
        if(!in_stateblock) {
            // match state beginning
            IF_STATE("end_state", CEndState)
            else IF_STATE("file_state", CFileState)
            else IF_STATE("regex_state", CRegexState)
            else IF_STATE("query_state", CQueryState)
            else IF_STATE("match_state", CMatchState)
            else IF_STATE("http_state", CHttpState)
            else IF_STATE("mail_state", CMailState)
            else IF_STATE("sms_state", CSmsState)
            else IF_STATE("script_state", CScriptState)
            else IF_STATE("shell_state", CShellState)
            else IF_STATE("struct_state", CStructureState)
            else IF_STATE("goto_state", CGotoState)
            else throw parser_error(file, syntax_error, counter);
        }
        else {
            if(is_matched("done", line, regmatch, 0, file, counter)) {
                st->set_nextState(fetch_int(line, 1, regmatch));
                continue;
            }
            else if(is_matched("error", line, regmatch, 0, file, counter)) {
                st->set_errorState(fetch_int(line, 1, regmatch));
                continue;
            }
            else if(is_matched("logpfx_double", line, regmatch, 0, file, counter)) {
                // logprefix "some string"
                regoff_t len = regmatch[1].rm_eo - regmatch[1].rm_so;
                st->set_logPrefix(line.substr(regmatch[1].rm_so, len));                
            }
            else if(is_matched("logpfx_single", line, regmatch, 0, file, counter)) {
                // logprefix 'some string' -- do not interpolate string later
                regoff_t len = regmatch[1].rm_eo - regmatch[1].rm_so;
                st->set_logPrefix(line.substr(regmatch[1].rm_so, len), false);                
            }
            else if(is_matched("endstate", line, regmatch, 0, file, counter)) {
                // end-of-state
                if(!st->verify()) {
                    delete st;
                    throw parser_error(file, invalid_state, counter);
                }
                if(sm->find(stateno) != sm->end()) {
                    delete st;
                    throw parser_error(file, duplicate_state, counter);
                }
                sm->insert(std::pair<int, CState*>(stateno, st));
                in_stateblock = false; // end-of-state
                continue;
            }
            else {
                // parse state-specific definitions 
                st->parse(line, file, counter);
                continue;
            }
        }
    }

    if(in_stateblock) throw parser_error(file, unfinished_state, counter);

    ifs.close();
    if(sm->empty()) {
        delete sm;
        sm = nullptr;
    }
    return sm;
}







