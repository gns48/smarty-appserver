/**
 * @file   httpstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 14:03:51 2013
 * 
 * @brief  CHttpState class implementation
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <curl/curl.h>
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "apputils.hpp"

namespace pt = boost::property_tree;
extern pt::ptree *cpt;                       // property tree: global configuration
extern const char *syntax_error;

// *********************************************************************
// *** libCURL staff
// *********************************************************************

//  libcurl callback to store HTTP reply
static int curlWriter(char *data, size_t size, size_t nmemb, std::string *result) {
    result->append(data, size*nmemb);
    return (int)(size * nmemb);
}

// *********************************************************************
// *** CHttpGetState
// *********************************************************************

CHttpState::CHttpState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "http"), m_url(""), 
    m_ucertenc("PEM"), m_pkeyenc("PEM"), m_pkeypswd(""),
    m_method(HTTPGET),
    m_outputvar(""), m_params(""), m_dumpfile(""), m_dumpflag(false) {};

CHttpState::~CHttpState() {};

/*
  450 http
      url "http://www.smarty.ru" # https also works
      [usercert 'path']      // user certificate
      [ucertenc 'PEM'|'DER'] // user certificate encoding: 'PEM' or 'DER'
      [cacert   'path']      // Certification Authority certificate   
      [pkey     'path']      // private key
      [pkeyenc  'PEM'|'DER'] // private key encoding: 'PEM' or 'DER'
      outputvar @httpresult # store output to the variable specified
      outputvar @httpresult # store output to the variable specified
      [parameters "function=pay\&amount=10.0\&"]
      ...
      [parameters "param18=18\&param19=148.5"]
      [method [GET|]POST]
      [addheader 'Accept: Yes']
      [addheader 'Reject: No']
      [file "path"]
      done 400
      error 500
      endstate
*/

int CHttpState::parse(const std::string& line, const std::string& file, unsigned counter) {
    regmatch_t regmatch[REGMATCH_COUNT];
    regoff_t len;
    
    if(is_matched("url", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_url = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("usercert", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_usercert = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("ucertenc", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_ucertenc = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("cacert", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_cacert = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("pkey", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_pkey = line.substr(regmatch[1].rm_so, len);
        // set key password (if defined in the [sslkeys] section)
        std::string cfkey = "sslkeys.";
        cfkey.append(m_pkey);
        if(cpt->find(cfkey) != cpt->not_found()) m_pkeypswd = cpt->get<std::string>(cfkey);
    }
    else if(is_matched("pkeyenc", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_pkeyenc = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("outvar_long", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_outputvar = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("outvar_short", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        std::string vname = "@";
        vname.append(boost::lexical_cast<std::string>(get_number()));
        vname.push_back('.');
        m_outputvar = vname + line.substr(regmatch[1].rm_so+1, len-1);
    }
    else if(is_matched("http_params", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_params.append(line.substr(regmatch[1].rm_so, len));
    }
    else if(is_matched("method_get", line, regmatch, 0, file, counter)) {
        // already done
    }
    else if(is_matched("method_post", line, regmatch, 0, file, counter)) {
        m_method = HTTPPOST;
    }
    else if(is_matched("addheader", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_headers.push_back(line.substr(regmatch[1].rm_so, len));
    }
    else if(is_matched("file", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_dumpfile = line.substr(regmatch[1].rm_so, len);
        m_dumpflag = true;
    }
    else throw parser_error(file, syntax_error, counter);
    return 0;
}

bool CHttpState::verify() {
    return
        m_url.length() > 7 &&
        (strncasecmp(m_url.c_str(), "http://", 7) == 0 ||
         strncasecmp(m_url.c_str(), "https://", 8) == 0) &&
        (m_ucertenc == "PEM" || m_ucertenc == "DER") &&
        (m_pkeyenc == "PEM" ||  m_pkeyenc == "DER") &&
        m_outputvar.length() > 0 &&
        get_errorState() > 0 &&
        get_nextState() > 0 &&
        get_errorState() != get_nextState();
}

int CHttpState::execute(const FCGX_Request *request, CAssigner* assigner) {
    CURLcode rc;
    CURL *curl_handle;
    struct curl_slist *chunk = NULL;  // custom header
    char errorBuffer[CURL_ERROR_SIZE];
    std::string curl_url;
    std::string curl_params;
    std::string curl_outstring;
     
    assigner->evaluate(m_url, curl_url, this);
    if(m_params.length()) {
        assigner->evaluate(m_params, curl_params, this);
        if(m_method == HTTPGET) curl_url += curl_params; // concatenate GET-URL
    }
    
    curl_handle = curl_easy_init();
    
    if(!curl_handle) {
        log_warning("%s:%s:%d: libCURL: error creating connection",
                    get_scriptName().c_str(), get_stateName().c_str(), get_number());
        return get_errorState();
    }
    
    rc = curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorBuffer);
    if(rc != CURLE_OK)
        log_error("%s:%s:%d: libCURL: error setting errorBuffer", get_scriptName().c_str(),
                  get_stateName().c_str(), get_number());
    
    rc = curl_easy_setopt(curl_handle, CURLOPT_URL, curl_url.c_str());
    if(rc != CURLE_OK)
        log_error("%s:%s:%d: libCURL: error setting URL: %s", get_scriptName().c_str(),
                  get_stateName().c_str(), get_number(), errorBuffer);

    if(m_method == HTTPPOST) {
        rc = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, curl_params.c_str());
        if(rc != CURLE_OK)
            log_error("%s:%s:%d: libCURL: error setting POST parameters: %s",
                      get_scriptName().c_str(), get_stateName().c_str(),
                      get_number(), errorBuffer);
    }
    
    rc = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curlWriter);
    if(rc != CURLE_OK)
        log_error("%s:%s:%d: libCURL: error setting writer: %s",
                  get_scriptName().c_str(), get_stateName().c_str(),
                  get_number(), errorBuffer);
    
    rc = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &curl_outstring);
    if(rc != CURLE_OK)
        log_error("%s:%s:%d: libCURL: error setting output string: %s",
                  get_scriptName().c_str(), get_stateName().c_str(),
                  get_number(), errorBuffer);

    // set custom header
    if(m_headers.size() > 0) {
        for(const auto &it : m_headers) chunk = curl_slist_append(chunk, it.c_str());
        rc = curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, chunk);
        if(rc != CURLE_OK)
            log_error("%s:%s:%d: libCURL: error setting custom HTTP headers: %s",
                      get_scriptName().c_str(), get_stateName().c_str(),
                      get_number(), errorBuffer);
    }

    // HTTPS (SSL) stuff
    if(strncasecmp(m_url.c_str(), "https://", 8) == 0) {
        if(m_usercert.size()) {
            rc = curl_easy_setopt(curl_handle, CURLOPT_SSLCERTTYPE, m_ucertenc.c_str());
            if(rc != CURLE_OK)
                log_error("%s:%s:%d: libCURL: error setting user' certificate encoding: %s",
                          get_scriptName().c_str(), get_stateName().c_str(),
                          get_number(), errorBuffer);
            rc = curl_easy_setopt(curl_handle, CURLOPT_SSLCERT, m_usercert.c_str());
            if(rc != CURLE_OK)
                log_error("%s:%s:%d: libCURL: error setting user' certificate: %s",
                          get_scriptName().c_str(), get_stateName().c_str(),
                          get_number(), errorBuffer);
        }
        if(m_cacert.size()) {
            rc = curl_easy_setopt(curl_handle, CURLOPT_CAINFO, m_cacert.c_str());
            if(rc != CURLE_OK)
                log_error("%s:%s:%d: libCURL: error setting CA certificate: %s",
                          get_scriptName().c_str(), get_stateName().c_str(),
                          get_number(), errorBuffer);
        }
        if(m_pkey.size()) {
            rc = curl_easy_setopt(curl_handle, CURLOPT_SSLKEY, m_pkey.c_str());
            if(rc != CURLE_OK)
                log_error("%s:%s:%d: libCURL: error setting private key: %s",
                          get_scriptName().c_str(), get_stateName().c_str(),
                          get_number(), errorBuffer);
            if(m_pkeypswd.size()) {
                rc = curl_easy_setopt(curl_handle, CURLOPT_KEYPASSWD, m_pkeypswd.c_str());
                if(rc != CURLE_OK)
                    log_error("%s:%s:%d: libCURL: error setting private password: %s",
                              get_scriptName().c_str(), get_stateName().c_str(),
                              get_number(), errorBuffer);
            }
        }
    }
    
    rc = curl_easy_perform(curl_handle);

    if(chunk) curl_slist_free_all(chunk);
    curl_easy_cleanup(curl_handle);

    if(rc != CURLE_OK) {
        log_warning("%s:%s:%d: libCURL: error processing request: %s",
                    get_scriptName().c_str(), get_stateName().c_str(),
                    get_number(), errorBuffer);
        return get_errorState();
    }
    
    assigner->assignLocal(m_outputvar, curl_outstring.c_str());

    if(m_dumpflag) {
        std::ofstream thefile(m_dumpfile.c_str());
        if(thefile.is_open()) {
            thefile << curl_outstring;
            thefile.close();
        }
        else log_warning("%s:%s:%d: %s: dump error",
                         get_scriptName().c_str(), get_stateName().c_str(),
                         get_number(), m_dumpfile.c_str());
    }
    
    return get_nextState();
}

