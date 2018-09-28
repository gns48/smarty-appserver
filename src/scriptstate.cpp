/**
 * @file   scriptstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:50:03 2013
 * 
 * @brief  CScriptState class implementation
 * 
 */

#include "config.h"
#include "cstate.hpp"
#include "parser.hpp"


// *********************************************************************
// *** CScriptState
// *********************************************************************

CScriptState::CScriptState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "script") { };

CScriptState::~CScriptState() {};

int CScriptState::parse(const std::string& line, const std::string& file, unsigned counter) {
//    regmatch_t regmatch[REGMATCH_COUNT];
    return 0;
}

bool CScriptState::verify() {
    return get_errorState() > 0 && get_nextState() > 0 && get_errorState() != get_nextState();
}

int CScriptState::execute(const FCGX_Request *request, CAssigner* assigner) {
    return get_nextState();
}
