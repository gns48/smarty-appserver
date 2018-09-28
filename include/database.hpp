/**
 * @file   database.hpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Wed Dec 25 00:02:38 2013
 * 
 * @brief  database query and results abstraction
 * 
 */

#ifndef __CDATABASE_HPP__
#define __CDATABASE_HPP__

#include <vector>
#include <string>
#include <pqxx/pqxx>

typedef std::vector<char*> qresult_t;

class CDatabase {
public:
    CDatabase() {};
    virtual ~CDatabase() {};
    virtual int connect() = 0;
    virtual qresult_t* query(const std::string& statement) = 0;
    virtual int disconnect() = 0;
};

class CPgDatabase: public CDatabase {
    std::string m_connectString;
    pqxx::connection* m_connection;    
public:
    CPgDatabase(const std::string& dbname,
                const std::string& user,
                const std::string& password,
                const std::string& host,
                const int port);
    virtual ~CPgDatabase();
    virtual int connect();
    virtual int disconnect();
    virtual qresult_t* query(const std::string& statement);
};

void addDBSection(const std::string& section);
CDatabase* getDatabase(const std::string& section);
void connectDBs();
void disconnectDBs();

#endif // #ifndef __CDATABASE_HPP__







