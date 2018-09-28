/**
 * @file   gotostate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Fri Feb 12 22:45:39 2016
 * 
 * @brief  goto state definition. Added after Mike's request
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
// *** CGotoState
// *********************************************************************

CGotoState::CGotoState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "goto")
{
}

CGotoState::~CGotoState() {
    for(auto &it : m_assignments) delete it;
}


int CGotoState::parse(const std::string& line, const std::string& file, unsigned counter) {
    m_assignments.push_back(parseAssignment(line, get_number(), file, counter));
    return 0;
}

bool CGotoState::verify() {
    if(get_errorState() == 0) set_errorState(get_nextState());
    return get_nextState() > 0;
}

int CGotoState::execute(const FCGX_Request *request, CAssigner* assigner) {
    unsigned i;
    
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





