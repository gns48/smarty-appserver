/**
 * @file   http.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Wed Mar 12 14:03:51 2014
 * 
 * @brief  HTTP protocol staff
 * 
 */

#ifndef __HTTP_HPP__
#define __HTTP_HPP__

char* HTTPstatus(const char* prefix, const int code, char *buf, size_t buflen);
std::string urlDecode(std::string &SRC);

#endif // #ifndef __HTTP_HPP__
