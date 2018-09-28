/**
 * @file   structstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Sat Mar 15 13:38:13 2014
 * 
 * @brief  Structured text parsing state (JSON or XML)
 */

#include "config.h"
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "apputils.hpp"

namespace pt = boost::property_tree;
extern const char *syntax_error;

// *********************************************************************
// *** CStructureState
// *********************************************************************

CStructureState::CStructureState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "structure"), m_sformat(FUNSET) {};

CStructureState::~CStructureState() {
    for(auto &it : m_assignments) delete it;
};

int CStructureState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    if(is_matched("match_var", line, regmatch, 0, file, counter)) {
        regoff_t len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_matchVar = line.substr(regmatch[1].rm_so, len);
        log_debug("%s:%s:%u: matched variable %s", file.c_str(), get_stateName().c_str(), counter,
                  m_matchVar.c_str());
    }
    else if(is_matched("format_xml", line, regmatch, 0, file, counter)) {
        set_sformat(FXML);
        log_debug("%s:%s:%u: xml format matched", file.c_str(), get_stateName().c_str(), counter);
    }
    else if(is_matched("format_json", line, regmatch, 0, file, counter)) {
        set_sformat(FJSON);
        log_debug("%s:%s:%u: json format matched", file.c_str(), get_stateName().c_str(), counter);
    }
    else m_assignments.push_back(parseAssignment(line, get_number(), file, counter));
    return 0;
}

bool CStructureState::verify() {
    return
        m_sformat != FUNSET && 
        get_errorState() > 0 &&
        get_nextState() > 0 &&
        get_errorState() != get_nextState() &&
        m_matchVar.length() > 0;
}

int CStructureState::execute(const FCGX_Request *request, CAssigner* assigner) {
    char *val = assigner->getValue(m_matchVar);
    int next = get_errorState();
    m_pt.clear(); // cleanup
    if(val) {
        try {
            std::stringstream ss;
            ss << val;
            if(m_sformat == FJSON) pt::read_json(ss, m_pt);
            else pt::read_xml(ss, m_pt);
            if(m_assignments.size()) {
                for(unsigned i = 0; i < m_assignments.size(); ++i) {
                    assigner->assign(m_assignments[i], this);
                }
            }
            next = get_nextState();
        }
        catch(pt::ptree_error& e) {
            log_warning("%s:%s:%d: %s:%s: structure parser error: %s", get_scriptName().c_str(),
                        get_stateName().c_str(), get_number(), m_matchVar.c_str(), val, e.what());
        }
        free(val);
    }
    else {
        log_warning("%s:%s:%d: %s: no value", get_scriptName().c_str(),
                    get_stateName().c_str(), get_number(), m_matchVar.c_str());        
    }
    
    return next;
}

char* CStructureState::getPropositional(const std::string& name) const {
    try {
        std::string val = m_pt.get<std::string>(name.substr(1), "");
        if(val.length() > 0) return strdup(val.c_str());
        else {
            log_warning("%s:%s:%d: %s: no value", get_scriptName().c_str(),
                        get_stateName().c_str(), get_number(), name.c_str());
        }
    }
    catch(pt::ptree_error& e) {
        log_warning("%s:%s:%d: %s: ptree access error: %s", get_scriptName().c_str(),
                    get_stateName().c_str(), get_number(), name.c_str(), e.what());
    }
    return nullptr;
}


