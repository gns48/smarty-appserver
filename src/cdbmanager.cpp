/**
 * @file   cdbmanager.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Mon Jan 27 16:45:04 2014
 * 
 * @brief  common database management routines
 */

#include "config.h"
#include "database.hpp"
#include "myexceptions.hpp"
#include "apputils.hpp"
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;
extern pt::ptree *cpt;                       // property tree: global configuration

static std::map<std::string, CDatabase*> dbMap;

// filled before fork
void addDBSection(const std::string& section) {
    dbMap[section] = nullptr;
}

CDatabase* getDatabase(const std::string& section) {
     const auto it = dbMap.find(section);
     if(it != dbMap.end()) return it->second;
     return 0;
}

// running after fork, in child process
void connectDBs() {
    for(auto &it : dbMap) {
        const std::string dbkey = it.first + ".dbtype";
        const std::string dbtype = cpt->get<std::string>(dbkey);
        
        if(dbtype == "postgresql") {
            std::string key = it.first;
            std::string dbname_key(it.first);  dbname_key.append(".dbname");
            std::string dbuser_key(it.first);  dbuser_key.append(".dbuser");
            std::string dbpswd_key(it.first);  dbpswd_key.append(".dbpswd");
            std::string dbhost_key(it.first);  dbhost_key.append(".dbhost");
            std::string dbport_key(it.first);  dbport_key.append(".dbport");

            std::string dbname = cpt->get<std::string>(dbname_key, "smarty");
            std::string dbuser = cpt->get<std::string>(dbuser_key, "smarty");
            std::string dbpswd = cpt->get<std::string>(dbpswd_key, "smarty");
            std::string dbhost = cpt->get<std::string>(dbhost_key, "localhost");
            int         dbport = cpt->get<int>(dbport_key, 5432);
            it.second = new CPgDatabase(dbname, dbuser, dbpswd, dbhost, dbport);
            it.second->connect();
            log_message("%s: %s@%s:%d: connected",  __func__, dbname.c_str(), dbhost.c_str(), dbport);
        }
        else {
            log_warning("%s: (yet) unknown database type: %s", __func__, dbtype.c_str());
            throw unsupported_feature(dbtype + ": unknown database type");
        }
    }
}

void disconnectDBs() {
    for(const auto &it : dbMap) delete it.second;
}

    
    
    























