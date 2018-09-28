/**
 * @file   cregex.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Wed Oct 23 01:02:57 2013
 * @brief  regex(3) wrappers definition
 */

#include "config.h"
#include <cstdio>
#include <iostream>
#ifdef HAVE_LIBBSD
#include <bsd/string.h>
#else
#include <cstring>
#endif
#include <boost/lexical_cast.hpp>
#include "cregex.hpp"

const size_t CRegex::m_ErrbufSize = 128;

// error messages
constexpr char regex_not_found[] = ": regex not found";

/**
   @fn CRegex();
   @brief default constructor
*/
CRegex::CRegex(): m_pattern(""), m_errbuf(new char[m_ErrbufSize]) {};

/**
   @fn CRegex(const char* reg, const int flags = 0)
   @brief constructor
   @param reg -- regex pattern
   @param flags -- see regcomp(3)
   @throw <std::runtime_error>
*/
CRegex::CRegex(const char* reg, const int flags) throw(std::runtime_error):
    m_pattern(reg),
    m_errbuf(new char[m_ErrbufSize])
{
    int rv = regcomp(&m_regex, reg, flags);
    if(rv) {
        regerror(rv, &m_regex, m_errbuf, m_ErrbufSize*sizeof(char));
        throw std::runtime_error(std::string("regcomp error: ") + std::string(m_errbuf));
    }
}

/**
   @fn ~CRegex()
   @brief destructor
*/
CRegex::~CRegex() {
    delete [] m_errbuf;
    regfree(&m_regex);
}

const char* CRegex::getError(const int rv, size_t *errbufSize) {
    regerror(rv, &m_regex, m_errbuf, m_ErrbufSize*sizeof(char));
    if(errbufSize) *errbufSize = m_ErrbufSize;
    return m_errbuf;
}


/* ========= CRegexCollection ========== */

/**
   @fn CRegexCollection();
   @brief default constructor
*/
CRegexCollection::CRegexCollection() {};

/**
   @fn CRegexCollection::CRegexCollection(const char* regfile)
   @brief constructor
   @param regfile file with regex patterns
   @throw <std::runtime_error>
   @throw <std::invalid_argument>
*/
CRegexCollection::CRegexCollection(const char* regfile) {
    fill(regfile);
}

/**
   @fn CRegexCollection::~CRegexCollection()
   @brief destructor
*/
CRegexCollection::~CRegexCollection() {
    for(const auto &it : m_regmap) delete it.second;
}

/**
   @fn void fill(const char* regfile);
   @brief fill collection from file
*/
void CRegexCollection::fill(const char* regfile) {
    char buf[512];
    FILE *fd;
    std::string name;
    size_t count;

    fd = ::fopen(regfile, "r");
    if(!fd) throw std::invalid_argument(std::string(regfile) + ": can not read!");
    count = 0;
    while(1) {
        count++;
        if(fgets(buf, sizeof(buf)-1, fd) == NULL) break;
        if(buf[0] == '#') continue;
        else if(buf[0] == '\n') continue;
        else {
            char *eol = strrchr(buf, '\n');
            if(!eol) {
                fclose(fd);
                throw std::runtime_error(std::string(regfile) +
                                         std::string(": no end-of-line, line: ") +
                                         boost::lexical_cast<std::string>(count));
            }
            *eol = '\0';
            char *split = strchr(buf, '=');
            if(!split) {
                fclose(fd);
                throw std::runtime_error(std::string(regfile) +
                                         std::string(": format error, line: ") +
                                         boost::lexical_cast<std::string>(count));
            }
            *split = '\0';
            m_regmap[std::string(buf)] = new CRegex(split+1, REG_EXTENDED);
        }
    }
    fclose(fd);
}

/**
   @fn const regex_t* CRegexCollection::get(const char* regname) const
   @brief getter
   @param regname - regex name
   @return compiled regex
*/
const regex_t* CRegexCollection::get(const char* regname) const {
    const auto it = m_regmap.find(regname);
    if(it != m_regmap.end()) return it->second->get();
    else {
        char errbuf[128];
        strncpy(errbuf, regname, sizeof errbuf);
        strlcat(errbuf, regex_not_found, sizeof errbuf);
        throw std::runtime_error(errbuf);
    }   
}

/**
   @fn void CRegexCollection::add(const char* name, const char* reg, const int flags)
   @brief add regex to collection
   @param name -- regex name
   @param reg -- regex pattern
   @param flags -- see regcomp(3)
   @return none
   @throw <std::runtime_error>
   @throw <std::invalid_argument>
*/
void CRegexCollection::add(const char* name, const char* reg, const int flags) {
    if(!name) throw std::invalid_argument("CRegexCollection::add: bad name");
    if(!reg) throw std::invalid_argument("CRegexCollection::add: bad regex");
    if(m_regmap.count(name)) throw std::runtime_error("CRegexCollection::add: duplicate name");
    m_regmap[name] = new CRegex(reg, flags);
}

/**
   @fn CRegex* operator [] (const std::string& regname)
   @brief returns collection element by name
   @param name -- regex name
   @return CRegex*
   @throw <std::runtime_error("regex not found")>
*/
CRegex* CRegexCollection::operator [] (const std::string& regname) const {
    const auto &it = m_regmap.find(regname);
    if(it != m_regmap.end()) return it->second;
    else throw std::runtime_error(regname + regex_not_found);
}

/**
   @fn CRegex* operator [] (const char* regname)
   @brief returns collection element by name
   @param name -- regex name
   @return CRegex*
   @throw <std::runtime_error("regex not found")>
*/
CRegex* CRegexCollection::operator [] (const char* regname) const {
    const auto &it = m_regmap.find(regname);
    if(it != m_regmap.end()) return it->second;
    else {
        char errbuf[128];
        strncpy(errbuf, regname, sizeof errbuf);
        strlcat(errbuf, regex_not_found, sizeof errbuf);
        throw std::runtime_error(errbuf);
    }
}

/**
   @fn std::ostream& operator << (std::ostream& os, const CRegexCollection& cr)
   @brief CRegexCollection object dumper
   @param os -- ostream reference
   @param cr -- object to dump reference
   @return os, as usual :)
*/
std::ostream& operator << (std::ostream& os, const CRegexCollection& cr) {
    for(const auto &it : cr.m_regmap)
        os << it.first << '=' << it.second->pattern() << std::endl;
    return os;
}



