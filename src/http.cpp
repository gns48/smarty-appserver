/**
 * @file   http.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Wed Mar 12 14:19:17 2014
 * @brief  HTTP protocol staff
 */

#include "config.h"
#include <cstdio>
#include <string>
#include "http.hpp"

static struct {
    int code;
    const char *message;
} HTTPreply [] = {
    // as the most frequently used
    {404, "Not Found"},
    {500, "Internal Server Error"},
    {200, "OK"},

    {100, "Continue"},
    {101, "Switching Protocols"},

    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},

    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Time-out"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {416, "Requested range not satisfiable"},
    {417, "Expectation Failed"},

    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Time-out"},
    {505, "HTTP Version not supported"}
};

char* HTTPstatus(const char* prefix, const int code, char *buf, size_t buflen) {
    size_t i;
    for(i = 0; i < sizeof(HTTPreply)/sizeof(HTTPreply[0]); i++) {
        if(code == HTTPreply[i].code) break;
    }
    if(i == sizeof(HTTPreply)/sizeof(HTTPreply[0])) i = 33; // 500 Internal Server Error
    snprintf(buf, buflen, "%s: %d %s\n", prefix, code, HTTPreply[i].message);
    return buf;
}

std::string urlDecode(std::string &SRC) {
    std::string ret("");
    char ch;
    unsigned i, ii;
    for(i = 0; i < SRC.length(); i++) {
        if(int(SRC[i])==37) {  // '%' found
            sscanf(SRC.substr(i+1,2).c_str(), "%x", &ii);
            ch=static_cast<char>(ii);
            ret.push_back(ch);
            i=i+2;
        }
        else ret.push_back(SRC[i]);
    }
    return ret;
}










