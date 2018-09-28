/**
 * @file   matchstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:42:07 2013
 * 
 * @brief  CMatchState class implementation
 * 
 */

#include "config.h"
#include <boost/lexical_cast.hpp>
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "apputils.hpp"

extern const char *syntax_error;

// *********************************************************************
// *** CMatchState
// *********************************************************************

CMatchState::CMatchState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "match") {};

CMatchState::~CMatchState() {
    for(auto &it : m_rexList) delete it;
};

int CMatchState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    if(is_matched("match_var", line, regmatch, 0, file, counter)) {
        regoff_t len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_matchVar = line.substr(regmatch[1].rm_so, len);
        log_debug("%s:%s:%u: matched variable %s", __func__, file.c_str(), counter, m_matchVar.c_str());
    }
    else if(is_matched("case_regex", line, regmatch, 0, file, counter)) {
        regoff_t len1 = regmatch[1].rm_eo - regmatch[1].rm_so;
        regoff_t len2 = regmatch[2].rm_eo - regmatch[2].rm_so;
        int st;
        std::string regex = line.substr(regmatch[1].rm_so, len1);
        std::string state = line.substr(regmatch[2].rm_so, len2);
        CRegex *rex = new CRegex(regex.c_str(), REG_EXTENDED);
        m_rexList.push_back(rex);
        m_stateList.push_back(st = boost::lexical_cast<int>(state));
        log_debug("%s:%s:%u: matched regex %s, state: %d",
                  __func__, file.c_str(), counter, regex.c_str(), st);
    }
    else throw parser_error(file, syntax_error, counter);
    return 0;
}

bool CMatchState::verify() {
    return
        get_errorState() > 0 && get_nextState() > 0 && get_errorState() != get_nextState() &&
        m_matchVar.length() > 0 && m_rexList.size() > 0 && m_stateList.size() == m_rexList.size();
}

int CMatchState::execute(const FCGX_Request *request, CAssigner* assigner) {
    char *val = assigner->getValue(m_matchVar);
    int state;
    if(val) {
        state = get_nextState();
        for(size_t i = 0; i < m_rexList.size(); ++i) {
            int rv = regexec(m_rexList[i]->get(), val, 0, 0, 0);
            if(!rv) {
                state = m_stateList[i];
                break;
            }
            else if(rv != REG_NOMATCH) {
                state = get_errorState();
                break;
            }
        }
        free(val);
    }
    else state = get_errorState();
    return state;
}
