#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>

namespace pt = boost::property_tree;


int main(int ac, char **av) {
    std::stringstream ss;
    pt::ptree pt;
    
    ss << av[1];
    pt::read_json(ss, pt);

    std::cout << pt.get<std::string>(av[2]) << std::endl;
}


    

