/**
 * Project: Smatry application server
 * Author: Gleb Semenov
 * Module: regex wrapper class declarationы
 * First edition: Tue Oct 15 19:43:56 MSK 2013
 */

#ifndef __CREGEX_HPP__
#define __CREGEX_HPP__

#include <map>
#include <string>
#include <iostream>
#include <stdexcept>
#include <regex.h>

#define REGMATCH_COUNT 20

/**
   CRegex class
   @brief posix-style regex wrapper
*/
   
class CRegex {
    static const size_t m_ErrbufSize;       /**< @brief error buffer size. internal constant */
    std::string m_pattern;                    /**< @brief saved pattern */
    regex_t m_regex;                        /**< @brief compiled regex */
    char *m_errbuf;                         /**< @brief compilation error text */
public:
    /**
       @fn CRegex();
       @brief default constructor
     */
    CRegex();
    /**
       @fn CRegex(const char* reg, const int flags = 0)
       @brief constructor
       @param reg -- regex pattern
       @param flags -- see regcomp(3)
       @throw <std::runtime_error>
     */
    CRegex(const char* reg, const int flags = 0) throw(std::runtime_error);
    /**
       @fn ~CRegex()
       @brief destructor
    */
    ~CRegex();
    /** 
        @brief getter
    */
    inline const regex_t* get() const { return &m_regex; }
    /**
       @fn inline const std::string& pattern() const
       @brief saved pattern getter
       @return saved pattern
    */
    inline const std::string& pattern() const { return m_pattern; }
    /**
       @fn const char* getError(const int rv, size_t *errbufSize);
       @brief returns error message for error code specified
       @param (in) const int rv -- error code
       @param (out) size_t *errbufSize -- pointer to buffer size. function return error
                                          buffer size if parameter is not null 
       @return error buffer
    */
    const char* getError(const int rv, size_t *errbufSize = 0);
    
};

/**
   CRegexCollection class
   @brief posix-style regex container
*/
class CRegexCollection {
    std::map<std::string, CRegex*> m_regmap; /// @brief regex container
public:
    /**
       @fn CRegexCollection();
       @brief default constructor
    */
    CRegexCollection();
    
    /**
       @fn CRegexCollection(const char* regfile);
       @brief constructor
       @param regfile file with regex patterns
       @throw <std::runtime_error>
       @throw <std::invalid_argument>
    */
    CRegexCollection(const char* regfile);
    /**
       @fn ~CRegexCollection();
       @brief destructor
    */
    ~CRegexCollection();

    /**
       @fn void fill(const char* regfile);
       @brief fill collection from file
    */
    void fill(const char* regfile);
    
    /**
       @fn inline const regex_t* get(const char* regname) const
       @brief getter
       @param regname - regex name
       @return compiled regex
    */
    const regex_t* get(const char* regname) const;
    
    /**
       @fn int add(const char* name, const char* reg, const int flags = 0)
       @brief add regex to collection
       @param name -- regex name
       @param reg -- regex pattern
       @param flags -- see regcomp(3), optional, default 0
       @return none
       @throw <std::runtime_error>
       @throw <std::invalid_argument>
    */
    void add(const char* name, const char* reg, const int flags = 0);

    /**
       @fn CRegex* operator [] (const std::string& regname)
       @brief returns collection element by name
       @param name -- regex name
       @return CRegex*
       @throw <std::runtime_error("regex not found")>
    */
    CRegex* operator [] (const std::string& regname) const;

    /**
       @fn CRegex* operator [] (const char* regname)
       @brief returns collection element by name
       @param name -- regex name
       @return CRegex*
       @throw <std::runtime_error("regex not found")>
    */
    CRegex* operator [] (const char* regname) const;
    
    /** @brief CRegexCollection object dumper declared as friend */
    friend std::ostream& operator<<(std::ostream& os, const CRegexCollection& cr);
};

/** 
    @fn std::ostream& operator << (std::ostream& os, const CRegexCollection& cr)
    @brief CRegexCollection object dumper
    @param os -- ostream reference
    @param cr -- object to dump reference
    @return os, as usual :)
*/
std::ostream& operator << (std::ostream& os, const CRegexCollection& cr);

#endif
    
    
    
    
