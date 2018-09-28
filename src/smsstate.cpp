/**
 * @file   smsstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:58:04 2013
 * 
 * @brief  CSmsState class implementation
 * 
 */

#include "config.h"
#include "cstate.hpp"
#include "parser.hpp"

// *********************************************************************
// *** CSmsState
// *********************************************************************

CSmsState::CSmsState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "sms") {};

CSmsState::~CSmsState() {};

int CSmsState::parse(const std::string& line, const std::string& file, unsigned counter) {
    return 0;
}

bool CSmsState::verify() {
    return get_errorState() > 0 && get_nextState() > 0 && get_errorState() != get_nextState();
}

int CSmsState::execute(const FCGX_Request *request, CAssigner* assigner) {
    return get_nextState();
}








