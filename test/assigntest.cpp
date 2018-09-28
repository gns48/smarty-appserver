/**
 * @file   assigntest.cpp
 * @author Gleb Semenov <gleb.semenov@gmail.com>
 * @date   Thu Dec  5 17:37:11 2013
 * 
 * @brief  assignment parsing utilities unit test
 * 
 * 
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <regex.h>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include "apputils.hpp"
#include "cregex.hpp"
#include "parser.hpp"

namespace pt = boost::property_tree;
pt::ptree cpt;                       // property tree: global configuration



int main(int ac, char **av) {
    int rv;
    
    if(ac < 3) {
        std::cout << "Usage: "
                  << av[0] << " <test assignments list> <state num>" << std::endl;
        return ENOENT;
    }

    rv = openRegexCollection();
    if(rv) return rv;
    
    try {
        std::vector<assignmentList_t*> assignments;
        std::string line;
        unsigned counter = 0;

        std::string fname = std::string(av[2]);
        std::ifstream ifs(fname.c_str());
        if(!ifs.is_open()) {
            std::cerr << "Can not open test data!" << std::endl;
            return ENOENT;
        }
        
        int state = boost::lexical_cast<int>(av[3]);
        
        while(getline(ifs, line)) {
            if(line.find('=') == std::string::npos) continue;  // empty line or other crap
            assignments.push_back(parseAssignment(line, state, fname, ++counter));
        }
        
        ifs.close();

        for(size_t i = 0; i < assignments.size(); i++) {
            auto it = assignments[i]->begin();
            while(it != assignments[i]->end()) {
                std::cout << *it << " ";
                ++it;
            }
            std::cout << std::endl;
            delete assignments[i];
        }
    }
    catch(parser_error& err) {
        std::cerr << "parser_error:" << err.whatFile()
                  << ":" << err.whatLine() << ": " << err.what() << std::endl;
        return ENOENT;
    }
    catch(std::runtime_error& err) {
        std::cerr << "std::runtime_error exception caught: " << err.what() << std::endl;
        return ENOENT;
    }
    catch(std::invalid_argument& err) {
        std::cerr << "invalid_argument exception caught!" << err.what() << std::endl;
        return EINVAL;
    }
    catch(boost::bad_lexical_cast &err) {
        std::cerr << "Can not interpret state number: " << err.what() << std::endl;
        return EINVAL;
    }
    return 0;
}



