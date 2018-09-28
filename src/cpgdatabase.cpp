/**
 * @file   cpgdatabase.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Tue Jan 28 00:23:09 2014
 * 
 * @brief  CPgDatabase class definition
 * 
 * 
 */

#include "config.h"
#include <cstring>
#include <boost/lexical_cast.hpp>
#include "database.hpp"
#include "apputils.hpp"

CPgDatabase::CPgDatabase(const std::string& dbname,
                         const std::string& user,
                         const std::string& password,
                         const std::string& host,
                         const int port) : CDatabase(), m_connection(nullptr)
{
    m_connectString = "user=";            m_connectString.append(user);
    m_connectString.append(" password="); m_connectString.append(password);
    m_connectString.append(" dbname=");   m_connectString.append(dbname);
    m_connectString.append(" host=");     m_connectString.append(host);
    m_connectString.append(" port=");     m_connectString.append(boost::lexical_cast<std::string>(port));
}

CPgDatabase::~CPgDatabase() {
    m_connection->disconnect();
    m_connection = nullptr;
}

int CPgDatabase::connect() {
    if(!m_connection) m_connection = new pqxx::connection(m_connectString.c_str());
    return 0;
}

int CPgDatabase::disconnect() {
    m_connection->disconnect();
    m_connection = nullptr;
    return 0;
}

qresult_t* CPgDatabase::query(const std::string& statement) {
    pqxx::work w(*m_connection);
    log_message("%s: statement: %s", __func__, statement.c_str());
    pqxx::result r = w.exec(statement);
    w.commit();
    if(r.size()) {
        const pqxx::tuple row = r[0];
        const int num_cols = row.size();
        log_message("%s: rows: %d, cols: %d", __func__, r.size(), num_cols);
        if(num_cols) {
            qresult_t *result = new qresult_t(num_cols);
            for(int colnum = 0; colnum < num_cols; ++colnum) {
                const pqxx::field field = row[colnum];
                if(field.size()) {
                    char *val = strdup(field.c_str());
                    log_message("%s: column: %d, val: %s", __func__, colnum, val);
                    (*result)[colnum] = val;
                }
                else {
                    (*result)[colnum] = nullptr;
                    log_warning("%s: column %d is NULL", __func__, colnum);
                }
            }
            return result;
        }
    }
    else {
        log_warning("%s: no data returned", __func__);
    }
    
    return 0;
}





