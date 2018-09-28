/**
 * @file   endstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:39:37 2013
 *
 * @brief  CEndState class implementation
 *
 */

#include "config.h"
#include <iostream>
#include "apputils.hpp"
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "http.hpp"

extern const char *syntax_error;

// *********************************************************************
// *** CEndState
// *********************************************************************

CEndState::CEndState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "end") { };

CEndState::~CEndState() {};

int CEndState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    regoff_t len;
    if(is_matched("data_single", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_outList.push_back(line.substr(regmatch[1].rm_so, len));
        m_interpretFlag.push_back(false);
    }
    else if(is_matched("data_double", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_outList.push_back(line.substr(regmatch[1].rm_so, len));
        m_interpretFlag.push_back(true);
    }

    else throw parser_error(file, syntax_error, counter);
    return 0;
}

bool CEndState::verify() {
    return
        get_errorState() == ENDSTATE && get_nextState() == ENDSTATE &&
        m_outList.size() == m_interpretFlag.size();
}

int CEndState::execute(const FCGX_Request *request, CAssigner* assigner) {
    size_t len = m_outList.size();

    if(len > 0) {
        std::string outstr("");
        for(size_t i = 0; i < len; ++i) {
            if(m_interpretFlag[i]) {
                assigner->evaluate(m_outList[i], outstr, this);
                FCGX_FPrintF(request->out, "%s\r\n", outstr.c_str());
            }
            else FCGX_FPrintF(request->out, "%s\r\n", m_outList[i].c_str());
        }
    }
    return ENDSTATE;
}




