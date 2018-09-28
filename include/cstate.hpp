#ifndef __CSTATE_HPP__
#define __CSTATE_HPP__

/**
 * @file   cstate.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Fri Oct 11 22:36:12 MSK 2013
 * 
 * @brief  Smatry application server. State declaration.
 */

#include <list>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <fcgiapp.h>
#include "cregex.hpp"

#define ENDSTATE (-1)

typedef std::list<std::string> assignmentList_t; /// parsed assignment

// forward declaration
class CAssigner;

/**
 * \brief Base CState class
 */

class CState {
    int m_number;              /// < @brief state number
    int m_errorState;          /// < @brief next state if error occured
    int m_nextState;           /// < @brief next state if success
    std::string m_scriptName;  /// < @brief current script name
    std::string m_stateName;   /// < @brief state name
    std::string m_logPrefix;   /// < @brief logging prefix
    bool m_pfxInterpretFlag;   /// < @brief logging prefix needs to be interpolated
public:
    explicit CState(int num, const std::string& scriptName, const char* stateName):
        m_number(num), m_errorState(-1), m_nextState(-1), m_scriptName(scriptName),
        m_stateName(stateName), m_logPrefix(""), m_pfxInterpretFlag(false) {};
    virtual ~CState() {};
    virtual int execute(const FCGX_Request *request, CAssigner* assigner) = 0;
    virtual int parse(const std::string& line, const std::string& file, unsigned counter) = 0;
    virtual bool verify() = 0;
    virtual char* getPropositional(const std::string& name) const {
        return nullptr;
    }
    
    inline void set_errorState(int num) { m_errorState = num; }
    inline void set_nextState(int num) { m_nextState = num; }
    inline int get_errorState() const { return m_errorState; }
    inline int get_nextState() const { return m_nextState; }
    inline const std::string& get_scriptName() const { return m_scriptName; }
    inline const std::string& get_stateName() const { return m_stateName; }
    inline int get_number() const { return m_number; }
    inline void set_logPrefix(const std::string& pfx, const bool flag=true) {
        m_logPrefix = pfx;
        m_pfxInterpretFlag = flag;
    }
    inline void set_logPrefix(const char* pfx, const bool flag=true) {
        m_logPrefix = pfx;
        m_pfxInterpretFlag = flag;
    }
    inline const std::string& get_logPrefix(bool *flag) const {
        *flag = m_pfxInterpretFlag;
        return m_logPrefix;
    }
};

/*
  100 regex 
      regex "([a-z][a-z0-9]+)\s*=\s*([0-9]+)"
      match @0.function
      done 110
      error 300
      name = $1
      value = $2
      endstate
*/
class CRegexState: public CState {
    std::string m_matchVar;   /// < @brief variable to match against
    CRegex      *m_regex;     /// < @brief 'compiled' regex
    regmatch_t  *m_regmatch;  /// < @brief matched subexpressions positions
    char        *m_substring[REGMATCH_COUNT]; /// < @brief matched substrings
    std::vector<assignmentList_t*> m_assignments; /// < @brief assignments list
    void setPattern(const char *pattern);
public:
    explicit CRegexState(const int stateno, const std::string& scriptName);
    virtual ~CRegexState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
    virtual char* getPropositional(const std::string& name) const;// override;
};

/*
  150 file
      file "/tmp/match.sl.output"
      done 200
      error 250
      data "Matched alphabetic: @100.value\n"
      endstate 
*/
class CFileState: public CState {
    std::string m_fileName;
    std::vector<std::string> m_outList;
    std::vector<bool> m_interpretFlag;
public:
    explicit CFileState(const int stateno, const std::string& scriptName);
    virtual ~CFileState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};

/*
  250 end
      data "Can not write file"
      endstate
*/
class CEndState: public CState {
    std::vector<std::string> m_outList;
    std::vector<bool> m_interpretFlag;
public:
    explicit CEndState(const int stateno, const std::string& scriptName);
    virtual ~CEndState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};

/*
  270 goto
      message = "we are in state 270"
      @500.exit = "passed through 270"
      done 500
      # may be the same as done state, may be absent
      # just for assignment error processing
      # if absent then default error state == done state
      [error 500]
      endstate
*/
class CGotoState: public CState {
    std::vector<assignmentList_t*> m_assignments;
public:
    explicit CGotoState(const int stateno, const std::string& scriptName);
    virtual ~CGotoState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};

/*
  110 match
      match @100.value
      case "[a-z][a-z]+" 150
      case "[0-9]+" 151
      done 152
      error 300
      endstate
*/
class CMatchState: public CState {
    std::string m_matchVar;
    std::vector<CRegex*> m_rexList;
    std::vector<int> m_stateList;
public:
    explicit CMatchState(const int stateno, const std::string& scriptName);
    virtual ~CMatchState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};

/*
  433 query
      db "postgres"
      query "select name, price from product where prod_id = 13"
      name = $name
      price = $price
      done 400
      error 500
      endstate
 */
class CQueryState: public CState {
    std::string m_dbsection;
    std::string m_query;
    std::vector<assignmentList_t*> m_assignments;
    std::vector<char*> *m_qResult;
    void clearQResult();
public:
    explicit CQueryState(const int stateno, const std::string& scriptName);
    virtual ~CQueryState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
    virtual char* getPropositional(const std::string& name) const;// override;
};

/*
  450 http
      url "http://www.smarty.ru" # https also works
      [usercert "path"]      // user certificate
      [ucertenc 'PEM'|'DER'] // user certificate encoding: 'PEM' or 'DER'
      [cacert   "path"]      // Certification Authority certificate   
      [pkey     'path']      // private key
      [pkeyenc  'PEM'|'DER'] // private key encoding: 'PEM' or 'DER'
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
class CHttpState: public CState {
    typedef enum {HTTPGET, HTTPPOST} httpmethod_t;
    std::string  m_url;
    std::string  m_usercert;
    std::string  m_ucertenc;
    std::string  m_cacert;
    std::string  m_pkey;
    std::string  m_pkeyenc;
    std::string  m_pkeypswd;
    httpmethod_t m_method;
    std::string  m_outputvar;
    std::string  m_params;
    std::list<std::string> m_headers;
    std::string  m_dumpfile;
    bool m_dumpflag;
public:
    explicit CHttpState(const int stateno,  const std::string& scriptName);
    virtual ~CHttpState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};


/*
  5322 mail
       [from "root@localhost"] ; MAIL FROM; optional, if absent the default from
                               ; configuration is used
       to "a\@a.com; @123.mailto..."    ; mandatory, recipients list
       [cc "b\@b.com; @123.mailcc..."]  ; optional, carbon copy list
       [attach 'file.ext'] ; optional attachment, mime type will
                           ; be defined by file extension
       [attach 'file.ext'] ; one more file to attach,
       subject "ARPAWOCKY, the RFC 527" ; mandatory, message subject
       data "Twas brillig, and the Protocols"
       data "   Did USER-SERVER in the wabe."
       data "All mimsey was the FTP,"
       data "  And the RJE outgrabe,"
       done 2822
       error 822
       endstate
 */
class CMailState: public CState {
    std::string m_server;
    std::string m_from;
    std::string m_to;
    std::string m_cc;
    std::string m_subject;
    std::list<std::string> m_attachments;
    std::list<std::string> m_data;
public:
    explicit CMailState(const int stateno, const std::string& scriptName);
    virtual ~CMailState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};

class CSmsState: public CState {
public:
    explicit CSmsState(const int stateno, const std::string& scriptName);
    virtual ~CSmsState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};


/*
  800 shell
      shell "do_something_with.sh @123.val @456.data"
      outputvar @result # store output to the variable specified
      done 880
      error 890
      endstate
*/
class CShellState: public CState {
    std::string m_command;
    std::string m_outputvar;
public:
    explicit CShellState(const int stateno, const std::string& scriptName);
    virtual ~CShellState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};

/*
  810 structure
      match @0.data
      format {XML|JSON}
      name = $state_code.a
      value = $state_code.b
      done 880
      error 890
      endstate
*/
class CStructureState: public CState {
    typedef enum {FUNSET, FXML, FJSON} sformat_t;
    std::string m_matchVar;                       /// < @brief variable to match
    sformat_t   m_sformat;                        /// < @brief format to parse (currently XML or JSON)
    boost::property_tree::ptree m_pt;            /// < @brief parsed property tree
    std::vector<assignmentList_t*> m_assignments; /// < @brief assignments list
    inline void set_sformat(sformat_t sf) { m_sformat = sf; }
public:
    explicit CStructureState(const int stateno, const std::string& scriptName);
    virtual ~CStructureState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
    virtual char* getPropositional(const std::string& name) const;// override;
};

/* for future development */

class CScriptState: public CState {
public:
    explicit CScriptState(const int stateno, const std::string& scriptName);
    virtual ~CScriptState();
    virtual int execute(const FCGX_Request *request, CAssigner* assigner);
    virtual int parse(const std::string&, const std::string&, unsigned);
    virtual bool verify();
};


#endif // #ifndef __CSTATE_HPP__



    
    


