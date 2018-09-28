/**
 * @file   querystate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 14:05:37 2013
 * 
 * @brief  CQueryState class implementation
 * 
 */

#include "config.h"
#include <cstring>
#include <boost/lexical_cast.hpp>
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "database.hpp"
#include "apputils.hpp"

namespace pt = boost::property_tree;
extern pt::ptree *cpt;                       // property tree: global configuration
extern const char *syntax_error;

// *********************************************************************
// *** CQueryState
// *********************************************************************

void CQueryState::clearQResult() {
    if(m_qResult) {
        for(const auto &it : *m_qResult) free(it);
        delete m_qResult;
        m_qResult = nullptr;
    }
}


CQueryState::CQueryState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "query"), m_dbsection(""), m_query(""), m_qResult(0) {};

CQueryState::~CQueryState() {
    clearQResult();
    for(auto &it : m_assignments) delete it;
};

int CQueryState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    regoff_t len;
    if(is_matched("db", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_dbsection = line.substr(regmatch[1].rm_so, len);

        if(cpt->find(m_dbsection) == cpt->not_found()) {
            log_error("%s:%u: '%s' database section is not defined in configuration!",
                      file.c_str(), counter, m_dbsection.c_str());
        }
        addDBSection(m_dbsection); // add section name to global database names list
    }
    else if(is_matched("query", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_query = line.substr(regmatch[1].rm_so, len);
    }
    else m_assignments.push_back(parseAssignment(line, get_number(), file, counter));
    return 0;
}

bool CQueryState::verify() {
    return
        get_errorState() > 0 && get_nextState() > 0 && get_errorState() != get_nextState() &&
        m_dbsection.length() > 0 && m_query.length() > 0;
}

char* CQueryState::getPropositional(const std::string& name) const {
    try {
        size_t num = boost::lexical_cast<size_t>(name.substr(1));
        if(m_qResult && num < m_qResult->size()) {
            if((*m_qResult)[num]) return strdup((*m_qResult)[num]);
            else return strdup("");
        }
    }
    catch(...) {
    }
    return nullptr;
}

int CQueryState::execute(const FCGX_Request *request, CAssigner* assigner) {
    unsigned i;
    clearQResult();    // cleanup from previous run
    try {
        std::string outq("");
        assigner->evaluate(m_query, outq, this);
        m_qResult = getDatabase(m_dbsection)->query(outq);
        for(i = 0; i < m_assignments.size(); ++i)
            assigner->assign(m_assignments[i], this);
    }
    catch(...) {
        return get_errorState();
    }
    return get_nextState();
}







