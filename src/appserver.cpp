/**
 * @file   appserver.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Wed Oct 23 09:04:22 2013
 *
 * @brief  main module
 */

#include "config.h"
#include <iostream>
#include <string>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "apputils.hpp"
#include "cregex.hpp"
#include "cstate.hpp"
#include "parser.hpp"
#include "preforked.hpp"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

extern const char *appserverVersion; // version
extern const char *appserverCommit;  // commit hash
extern const char *configFileName;   // default configuration file name
extern const char *scriptDirDefault; // in templates.cpp

pt::ptree *cpt = nullptr;  // property tree: global configuration

/**
 * in the configuration file we need to specify all scripts to parse as:
 * scriptName = scriptFile:entryPoint
 * scriptName SHOULD be unique but scriptFile MAY not
 * So we may have more that one scriptName with the same scriptFile and different
 * entryPoints
 */
scriptMap_t allScripts;              /// all parsed scripts (scriptName -> scriptMap)
scriptMap_t allScriptsByFile;        /// all parsed script files (scriptFile -> scriptMap)
entryMap_t  scriptEntries;           /// entry points (scriptName -> entryPoint)

void freeAllScripts() {
    for(const auto &sit : allScripts) {
        std::for_each(sit.second->begin(), sit.second->end(),
                      [](const std::pair<const int, CState*>& pair) { delete pair.second; });
        delete sit.second;
    }
}

int main(int ac, char** av) {
    int rv = 0;

    // command line option variables
    short listenPort;
    bool daemon_mode;
    bool debug_mode;
    std::string pidfile;
    std::string configPath;
    std::string rexLibPath;

    // boost::program_options staff
    po::options_description op_cmdline("Allowed options");
    op_cmdline.add_options()
        ("help,h", "produce help message")
        ("version,v", "print version")
        ("config,c",  po::value<std::string>(&configPath), "configuration file to use")
        ("listen-port,p", po::value<short>(&listenPort)->default_value(4800), "fast CGI listen port")
        ("daemon,d",
         po::value<bool>(&daemon_mode)->zero_tokens()->default_value(false)->implicit_value(true),
         "be a daemon after start")
        ("pidfile,f", po::value<std::string>(&pidfile), "pid file path (for daemon mode)")
        ("debug,g",
         po::value<bool>(&debug_mode)->zero_tokens()->default_value(false)->implicit_value(true),
         "run in debug mode")
        ("regex-lib,r", po::value<std::string>(&rexLibPath), "regex library to use");
    po::variables_map vm;
    
    try {
        po::store(po::parse_command_line(ac, av, op_cmdline), vm);
        po::notify(vm);
    }
    catch(po::error &err) {
        std::cerr << "command line option error: " << err.what() << std::endl;
        std::cout << op_cmdline << std::endl;
        return EINVAL;
    }

    if(vm.count("help")) {
        std::cout << appserverVersion << std::endl
                  << op_cmdline << std::endl;
        return 0;
    }
    if(vm.count("version")) {
        std::cout << appserverVersion << std::endl;
        std::cout << appserverCommit << std::endl;
        return 0;
    }

    // Configuration file staff -- boost:: property tree

    if(vm.count("config")) {
        fs::path p(configPath);
        if(!(fs::exists(p) && fs::is_regular_file(p))) {
            std::cerr << "Can not find " << p << ", or it is not a regular file" << std::endl;
            return ENOENT;
        }
    }
    else if(!findConfigFile(configPath, configFileName)) {
        std::cerr << "Can not find default configuration file" << std::endl;
        return ENOENT;
    }

    cpt = new pt::ptree;

    read_ini(configPath, *cpt);
    if(vm.count("listen-port"))
        cpt->put("common.listen_port",  boost::lexical_cast<std::string>(listenPort));
    if(vm.count("pidfile"))
        cpt->put("common.pidfile", pidfile);
    cpt->put("runtime.debug_mode", boost::lexical_cast<std::string>(debug_mode));
    cpt->put("runtime.daemon_mode", boost::lexical_cast<std::string>(daemon_mode));

    if(vm.count("regex-lib")) rv = openRegexCollection(rexLibPath.c_str());
    else rv = openRegexCollection();
    if(rv) return rv;

    try {
        std::string spath = cpt->get<std::string>("common.scriptdir", scriptDirDefault) + "/";
//        for(const auto &v : cpt->get_child("script")) {
        BOOST_FOREACH(const pt::ptree::value_type &v, cpt->get_child("script")) {
            if(allScripts.find(v.first.data()) != allScripts.end()) {
                throw pt::ptree_error(std::string(v.first.data()) + ": duplicate name");
            }

            std::string namestr = v.second.data();
            std::string::size_type n = namestr.rfind(":");
            if(n == std::string::npos) {
                throw std::runtime_error(namestr + ": [script] section format error");
            }

            // copy "pure" file name
            std::string fname = namestr.substr(0, n);

            // store entry point
            scriptEntries[v.first.data()] = boost::lexical_cast<int>(namestr.substr(n + 1));

            // check if file already parsed
            const auto it = allScriptsByFile.find(fname);
            if(it != allScriptsByFile.end()) {
                // yes, just store pointer to already existed stateMap in allScripts
                allScripts[v.first.data()] = it->second;
            }
            else {
                // no, parse file and store pointer into the both maps
                allScripts[v.first.data()] = allScriptsByFile[fname] = parseScript(spath + fname);
            }
        }
    }
    catch(pt::ptree_error &e) {
        std::cerr << configPath << ": [script] section parse error: " << e.what()  << std::endl;
        return EINVAL;
    }
    catch(parser_error &e) {
        std::cerr << "parser: "
                  << e.whatFile() << ":"
                  << e.whatLine() << ": " << e.what() << std::endl;
        return ENOENT;
    }
    catch(std::runtime_error &e) {
        std::cerr << "runtime error: " << e.what() << std::endl;
        return EINVAL;
    }
    catch(std::exception &e) {
        std::cerr << "(yet) unknown parser exception caught: " << e.what() << std::endl;
        return EINVAL;
    }

    if(allScripts.empty()) {
        std::cerr << "No scripts parsed! The appserver has nothing to do." << std::endl;
        return ENOENT;
    }

    // be a daemon if told, daemonize initializes logging also
    extern const char* pidFileDefault; // in templates.cpp
    std::string pfile = cpt->get("common.pidfile", pidFileDefault);

    if(daemon_mode) daemonize();

    init_syslog(cpt->get<int>("logging.facility", 7),
                cpt->get<std::string>("logging.logid", "appserver").c_str(),
                cpt->get<bool>("runtime.debug_mode"));

    if(daemon_mode) {
        rv = write_pid(pfile.c_str());
        if(rv) log_error("%s: can not write pid file: %d (%s)", __func__, rv, strerror(rv));
    }

    log_debug("Parent control process started");

    runPreforked();
    
    freeAllScripts();
    freeRegexCollection();
    
    unlink(pfile.c_str());

    delete cpt;
    
    log_warning("Parent control process exiting...");
    closelog();
    
    return 0;
}










