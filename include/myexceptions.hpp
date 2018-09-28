/**
 * @file   myexceptions.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 21:30:35 2013
 * 
 * @brief  All the exceptions we throw :)
 * 
 */

#ifndef __MYEXCEPTIONS_HPP__
#define __MYEXCEPTIONS_HPP__

#include <stdexcept>
#include <string>
#include <libmemcached/memcached.h>

class parser_error: public std::runtime_error {
    std::string what_file;
    unsigned int what_line;
public:
    explicit parser_error(const std::string& file,
                          const std::string& what_arg,
                          const unsigned int line=0):
        runtime_error(what_arg), what_file(file), what_line(line) {}
    explicit parser_error(const std::string& file,
                          const char* what_arg,
                          const unsigned int line=0):
        runtime_error(what_arg), what_file(file), what_line(line) {}
    virtual ~parser_error() throw () {}
    const std::string& whatFile() const { return what_file; }
    unsigned int whatLine() const { return what_line; }
};

class undefined_value: public std::runtime_error {
public:
    explicit undefined_value(const std::string& what_arg): runtime_error(what_arg) {};
    explicit undefined_value(const char* what_arg): runtime_error(what_arg) {};
    virtual ~undefined_value() throw () {};
};

class unsupported_feature: public std::logic_error {
public:
    explicit unsupported_feature(const std::string& what_arg): logic_error(what_arg) {};
    explicit unsupported_feature(const char* what_arg): logic_error(what_arg) {};
    virtual ~unsupported_feature() throw () {};
};

class memcache_error: public std::runtime_error {
    memcached_return_t what_rv;
public:
    explicit memcache_error(const std::string& what_arg, memcached_return_t rv):
        runtime_error(what_arg), what_rv(rv) {};
    explicit memcache_error(const char* what_arg, memcached_return_t rv): runtime_error(what_arg), what_rv(rv) {};
    virtual ~memcache_error() throw () {};
    memcached_return_t code() const { return what_rv; }
};

#endif // #ifndef __MYEXCEPTIONS_HPP__
