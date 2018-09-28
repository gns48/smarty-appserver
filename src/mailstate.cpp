/**
 * @file   mailstate.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Dec 10 13:59:35 2013
 * 
 * @brief  CMailState class implementation
 * 
 */

#include "config.h"
#include <time.h>
#include <cstdlib>
#include <cerrno>
#include <uuid.h>
#include <curl/curl.h>
#include "cstate.hpp"
#include "parser.hpp"
#include "cassigner.hpp"
#include "apputils.hpp"

namespace pt = boost::property_tree;
extern pt::ptree *cpt;                       // property tree: global configuration

extern const char *syntax_error;

static const std::string CRLF("\r\n");

// *********************************************************************
// *** locals
// *********************************************************************

static std::vector<std::string> *smtpMessage = nullptr;

static size_t payload_writer(void *ptr, size_t size, size_t nmemb, void *userp) {
    size_t *lines = (size_t*)userp;
    
    if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) return 0;
    
    if(*lines < smtpMessage->size()) {
        const char *data = (*smtpMessage)[*lines].c_str();
        size_t      dlen = (*smtpMessage)[*lines].size();
        memcpy(ptr, data, dlen);
        (*lines)++;
        return dlen;
    }
    
    return 0;
}

int rfcDate(char *dstr, size_t dsize) {
    static const char *lctime = "LC_TIME";
    static const char *rfcfmt = "%a, %d %b %Y %T %z";
    time_t t;
    struct tm ts;
    
    char *old_lctime = getenv(lctime);
    setenv(lctime, "C", 1);

    t = time(NULL);
    localtime_r(&t, &ts);
    
    if(strftime(dstr, dsize, rfcfmt, &ts) == 0)
        log_error("%s:%s: strftime error: %d (%s)", __FILE__, __func__, errno, strerror(errno));
    
    if(old_lctime) setenv(lctime, old_lctime, 1);
    else unsetenv(lctime);

    return 0;
}


// *********************************************************************
// *** CMailState
// *********************************************************************

/*
  5322 mail
       [from "root@localhost"] ; MAIL FROM; optional, if absent the default from
                               ; configuration is used
       to "a\@a.com; @123.mailto..."    ; mandatory, recipients list
       [cc "b\@b.com; @123.mailcc..."]  ; optional, carbon copy list
       [attach 'file.ext'] ; optional attachment, mime type will
                           ; be defined by file extension
       [attach 'file.ext'] ; one more file to attach,
       subject "ARPAWOCKY, the RFC 527" ; mandatory, smtp subject
       data "Twas brillig, and the Protocols"
       data "   Did USER-SERVER in the wabe."
       data "All mimsey was the FTP,"
       data "  And the RJE outgrabe,"
       done 2822
       error 822
       endstate
*/

CMailState::CMailState(const int stateno, const std::string& scriptName):
    CState(stateno, scriptName, "mail"), m_server(""), m_from(""), m_to(""), m_cc(""), m_subject("")
{
    m_server = cpt->get<std::string>("smtp.mailserver", ""); // put DNS-resolved MX host as default
}

CMailState::~CMailState() {};

int CMailState::parse(const std::string& line, const std::string& file, unsigned counter) { 
    regmatch_t regmatch[REGMATCH_COUNT];
    regoff_t len;
    
    if(is_matched("mfrom", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_from = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("mto", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_to = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("mcc", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_cc = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("subject", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_subject = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("attach", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_attachments.push_back(line.substr(regmatch[1].rm_so, len));
        m_subject = line.substr(regmatch[1].rm_so, len);
    }
    else if(is_matched("data_double", line, regmatch, 0, file, counter)) {
        len = regmatch[1].rm_eo - regmatch[1].rm_so;
        m_data.push_back(line.substr(regmatch[1].rm_so, len));
    }
    else throw parser_error(file, syntax_error, counter);

    if(m_from.length() == 0) m_from = cpt->get<std::string>("smtp.sender", "");
    return 0;
}

bool CMailState::verify() {
    return
        get_errorState() > 0 && get_nextState() > 0 && get_errorState() != get_nextState() &&
        m_server.size() > 0 &&
        m_from.size() > 0 &&
        m_to.size() > 0 &&
        m_subject.size() > 0 &&
        m_data.size() > 0;    
}

int CMailState::execute(const FCGX_Request *request, CAssigner* assigner) {
    CURLcode rc;
    CURL *curl_handle;
    char errorBuffer[CURL_ERROR_SIZE];

    std::string mxURL = "smtp://" + m_server;
    std::string mxFrom;
    std::string mxTo;
    std::string mxCC;
    
    struct curl_slist *recipients = NULL;
    
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

    // server
    rc = curl_easy_setopt(curl_handle, CURLOPT_URL, mxURL.c_str());
    if(rc != CURLE_OK)
        log_error("%s:%s:%d: libCURL: error setting URL: %s", get_scriptName().c_str(),
                  get_stateName().c_str(), get_number(), errorBuffer);
    // sender
    if(m_from.size() == 0) m_from = cpt->get<std::string>("smtp.from", "root@localhost");
    else assigner->evaluate(m_from, mxFrom, this);
    rc = curl_easy_setopt(curl_handle, CURLOPT_MAIL_FROM, mxFrom.c_str());
    if(rc != CURLE_OK)
        log_error("%s:%s:%d: libCURL: error setting mail sender: %s", get_scriptName().c_str(),
                  get_stateName().c_str(), get_number(), errorBuffer);
    //recipients
    assigner->evaluate(m_to, mxTo, this);
    assigner->evaluate(m_cc, mxCC, this);
    recipients = curl_slist_append(recipients, mxTo.c_str());
    recipients = curl_slist_append(recipients, mxCC.c_str());
    rc = curl_easy_setopt(curl_handle, CURLOPT_MAIL_RCPT, recipients);
    if(rc != CURLE_OK)
        log_error("%s:%s:%d: libCURL: error setting mail recipients list: %s", get_scriptName().c_str(),
                  get_stateName().c_str(), get_number(), errorBuffer);

    // begin assembling message body
    smtpMessage = new std::vector<std::string>;
    size_t lines_read = 0;
    std::string tmpstr;
    char tmpbuf[128];
    uuid_t uuid_id;

    // date
    tmpstr = "Date: ";
    rfcDate(tmpbuf, sizeof(tmpbuf));
    tmpstr.append(tmpbuf); tmpstr.append(CRLF);
    smtpMessage->push_back(tmpstr);
    // From
    tmpstr = "From: "; tmpstr.append(mxFrom); tmpstr.append(CRLF);
    smtpMessage->push_back(tmpstr);
    // To
    tmpstr = "To: "; tmpstr.append(mxTo); tmpstr.append(CRLF);
    smtpMessage->push_back(tmpstr);
    // CC
    tmpstr = "CC: "; tmpstr.append(mxCC); tmpstr.append(CRLF);
    smtpMessage->push_back(tmpstr);

    // to be RFC pedant, add message ID as uuid@from-domain
    tmpstr = "Message-ID: ";
    uuid_generate(uuid_id);
    uuid_unparse(uuid_id, tmpbuf);
    tmpstr.append(tmpbuf);
    
    std::string::size_type at_pos = mxFrom.rfind('@');
    tmpstr.append(mxFrom.substr(at_pos));
    tmpstr.append(CRLF);
    smtpMessage->push_back(tmpstr);
    
    // subject
    std::string subj;
    assigner->evaluate(m_subject, subj, this);
    tmpstr = "Subject: "; tmpstr.append(subj); tmpstr.append(CRLF);
    smtpMessage->push_back(tmpstr);
    
    // add one more empty line
    tmpstr.append(CRLF);
    smtpMessage->push_back(tmpstr);
    // message text
    for(const auto &it: m_data) {
        assigner->evaluate(it, tmpstr, this);
        tmpstr.append(CRLF);
        smtpMessage->push_back(tmpstr);
    }
    // We're using a callback function to specify the payload (the headers and body of the message)
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, payload_writer);
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, &lines_read);
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);

    
     /* Send the message */
    rc = curl_easy_perform(curl_handle);

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl_handle);
    
    if(rc != CURLE_OK) {
        log_warning("%s:%s:%d: libCURL: error sending mail: %s",
                    get_scriptName().c_str(), get_stateName().c_str(),
                    get_number(), errorBuffer);
        return get_errorState();
    }

    return get_nextState();
}





