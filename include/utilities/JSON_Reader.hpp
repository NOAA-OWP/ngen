#pragma once

#include <boost/property_tree/json_parser.hpp>
#include <string>

// Trivial wrapper to functionally *return* the parsed ptree, rather
// than taking an output argument
inline boost::property_tree::ptree ptree_from_json_file(std::string const& path) {
    boost::property_tree::ptree ptree;
    boost::property_tree::json_parser::read_json(path, ptree);
    return ptree;
}
