/**
 * @file   regexstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:32:46 2013
 * 
 * @brief  CRegexState class definition
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
// *** CRegexState
// *********************************************************************

CRegexState::CRegexState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "regex"),
    m_matchVar(""), m_regex(0), m_regmatch(new regmatch_t[REGMATCH_COUNT])
{
    for(int i = 0; i < REGMATCH_COUNT; ++i) m_substring[i] = 0;
}

CRegexState::~CRegexState() {
    if(m_regex) delete m_regex;
    delete [] m_regmatch;
    for(auto &it : m_assignments) delete it;
}

void CRegexState::setPattern(const char *pattern) {
    m_regex = new CRegex(pattern, REG_EXTENDED);
}

int CRegexState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    regoff_t len;
    if(is_matched("regex_double", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        setPattern(line.substr(regmatch[1].rm_so, len).c_str());
        log_debug("%s:%s:%u: matched regex %s", __func__, file.c_str(), counter,
                  line.substr(regmatch[1].rm_so, len).c_str());
    }
    else if(is_matched("match_var", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_matchVar = line.substr(regmatch[1].rm_so, len);
        log_debug("%s:%s:%u: matched variable %s", __func__, file.c_str(), counter, m_matchVar.c_str());
    }
    else m_assignments.push_back(parseAssignment(line, get_number(), file, counter));
    return 0;
}

bool CRegexState::verify() {
    return
        get_errorState() > 0 && get_nextState() > 0 && get_errorState() != get_nextState() &&
        m_matchVar.length() > 0 && m_regex != nullptr;
}

char* CRegexState::getPropositional(const std::string& name) const {
    int num = boost::lexical_cast<int>(name.substr(1));
    // strdup is to conform memcached return policy
    if(num < REGMATCH_COUNT && m_substring[num]) return strdup(m_substring[num]);
    return nullptr;
}

int CRegexState::execute(const FCGX_Request *request, CAssigner* assigner) {
    unsigned i;
    int rv;
    char *val = assigner->getValue(m_matchVar);
    
    // cleanup from previour run
    for(i = 0; i < REGMATCH_COUNT; i++) {
        m_regmatch[i].rm_so = m_regmatch[i].rm_eo = -1;
        if(m_substring[i]) {
            delete [] m_substring[i];
            m_substring[i] = 0;
        }
    }

    if(!val) {
        log_warning("%s:%s:%d: %s not found", get_scriptName().c_str(),
                    get_stateName().c_str(), get_number(), m_matchVar.c_str());
        return get_errorState();
    }

/*    
    FCGX_FPrintF(request->out, "%s:%s:%d: %s = %s\n", get_scriptName().c_str(),
                 get_stateName().c_str(), get_number(), m_matchVar.c_str(), val);
*/    
    rv = regexec(m_regex->get(), val, REGMATCH_COUNT, m_regmatch, 0);
    if(rv) {
        log_warning("%s:%s:%d:%s: %s not matched: %s", get_scriptName().c_str(),
                    get_stateName().c_str(), get_number(), m_matchVar.c_str(), val,
                    m_regex->getError(rv, nullptr));
        free(val);
        return get_errorState();
    }
    
    for(i = 0; i < REGMATCH_COUNT && m_regmatch[i].rm_so >= 0; i++) {
        size_t len = m_regmatch[i].rm_eo - m_regmatch[i].rm_so;
        m_substring[i] = new char [len+1];
        memcpy(m_substring[i], val + m_regmatch[i].rm_so, len);
        m_substring[i][len] = '\0';
/*        
        FCGX_FPrintF(request->out, "%s:%s:%d: $%d = %s (%u)\n", get_scriptName().c_str(),
                     get_stateName().c_str(), get_number(), i, m_substring[i], len);
*/
    }

    free(val);
    
    if(m_assignments.size()) {
        try {
            for(i = 0; i < m_assignments.size(); ++i) {
                assigner->assign(m_assignments[i], this);
            }
        }
        catch(...) {
            return get_errorState();
        }
    }
    return get_nextState();
}


