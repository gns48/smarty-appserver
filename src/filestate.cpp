/**
 * @file   filestate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:37:28 2013
 * 
 * @brief  CFileState class definition
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "apputils.hpp"

extern const char *syntax_error;

// *********************************************************************
// *** CFileState
// *********************************************************************

CFileState::CFileState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "file"), m_fileName("") {};

CFileState::~CFileState() {};

int CFileState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    regoff_t len;
    if(is_matched("file", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_fileName = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("data_single", line, regmatch, 0, file, counter)) {
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

bool CFileState::verify() {
    return
        get_errorState() > 0 && get_nextState() > 0 && get_errorState() != get_nextState() &&
        m_fileName.length() > 0;
}


int CFileState::execute(const FCGX_Request *request, CAssigner* assigner) {
    std::string outstr("");
    std::ofstream thefile(m_fileName.c_str());
    if(thefile.is_open()) {
        for(size_t  i = 0; i < m_outList.size(); ++i) {
            if(m_interpretFlag[i]) {
                assigner->evaluate(m_outList[i], outstr, this);
                thefile << outstr << std::endl;
            }
            else thefile << m_outList[i] << std::endl;
        }
        thefile.close();
    }
    else {
        return get_errorState();
    }

    return get_nextState();
}


