/**
 * @file   shellstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:55:44 2013
 * 
 * @brief  CShellState class implementation
 * 
 */

#include "config.h"
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "apputils.hpp"

extern const char *syntax_error;

// *********************************************************************
// *** CShellState
// *********************************************************************

CShellState::CShellState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "shell"), m_command(""), m_outputvar("") {};

CShellState::~CShellState() {};

int CShellState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    regoff_t len;
    if(is_matched("shell", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_command = line.substr(regmatch[1].rm_so, len);
//        std::cout << __func__ << ": " << line << ": " << m_command << std::endl;
    }
    else if(is_matched("outvar_long", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_outputvar = line.substr(regmatch[1].rm_so, len);
//	std::cout << __func__ << ": " << line << ": " << "outvar_long: " << m_outputvar << std::endl;
    }
    else if(is_matched("outvar_short", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        std::string vname = "@";
        vname += boost::lexical_cast<std::string>(get_number());
        vname += ".";
        m_outputvar = vname + line.substr(regmatch[1].rm_so+1, len-1);
//	std::cout << __func__ << ": " << line << ": " << "outvar_short: " << m_outputvar << std::endl;
    }
    else {
//	std::cout << __func__ << ": " << line << std::endl;
	throw parser_error(file, syntax_error, counter);
    }
    return 0;
}

bool CShellState::verify() {
    return
        get_errorState() > 0 && get_nextState() > 0 && m_command.length() && m_outputvar.length() > 0;
}

int CShellState::execute(const FCGX_Request *request, CAssigner* assigner) {
    std::string command ("");
    assigner->evaluate(m_command, command, this);
    FILE *in = ::popen(command.c_str(), "r");
    if(in) {
        const size_t N = 1024;
        std::string result("");
        while (true) {
            std::vector<char> buf(N);
            size_t read = ::fread((void*)&buf[0], 1, N, in);
            if(read) result.append(buf.begin(), buf.end());
            if(read < N) break; 
        }
        pclose(in);
        assigner->assignLocal(m_outputvar, result.c_str());
        return get_nextState();    
    }

    return get_errorState();
}

