#include <iostream>
#include <fstream>
#include <cerrno>
#include <boost/property_tree/ptree.hpp>
#include "cregex.hpp"
#include "parser.hpp"

namespace pt = boost::property_tree;
pt::ptree cpt;                       // property tree: global configuration

int main(int ac, char **av) {
    int rv;
 
    if(ac < 22) {
        std::cout << "Usage: " << av[0] << " <test list>" << std::endl;
        return ENOENT;
    }

    rv = openRegexCollection();
    if(rv) return rv;

try {
        std::string line;
        const std::string file(av[2]);
        std::ifstream ifs(file.c_str());
        
        if(!ifs.is_open()) {
            std::cerr << "Can not open test data!" << std::endl;
            return ENOENT;
        }
        regmatch_t regmatch[REGMATCH_COUNT];
        unsigned counter = 1;
        while(getline(ifs, line)) {
            size_t pos = line.find('=');
            if(pos == std::string::npos) continue;
            const std::string& name = line.substr(0, pos);
            bool result = is_matched(name.c_str(), line.substr(pos+1), regmatch,
                                     0, file, counter);
            std::cout << file << ":" << counter << ":" << name << ": ";
            if(!result) std::cout << "not ";
            std::cout << "matched";
            int i = 0;
            while(i < REGMATCH_COUNT && regmatch[i].rm_so >= 0) {
                std::cout << ": "
                          <<  line.substr(pos+1+regmatch[i].rm_so,
                                          regmatch[i].rm_eo - regmatch[i].rm_so);
                i++;
            }
            std::cout << std::endl;
            counter++;
            if(!result) return EINVAL;
        }
        ifs.close();
    }
    catch(std::runtime_error& err) {
        std::cerr << "std::runtime_error exception caught: " << err.what() << std::endl;
        return ENOENT;
    }
    catch(std::invalid_argument& err) {
        std::cerr << "invalid_argument exception caught!" << err.what() << std::endl;
        return EINVAL;
    }
    return 0;
}

 
