// Copyright 2018 Open GEE Contributors
#include "serverdbUtilities.h"
#include <string>
#include <regex>

std::string ServerdbUtilities::ParseValidJSON(const std::string& javascript_dict) {
    std::regex fix_dictionary_names("([A-Za-z]+)\\s:");
    std::string json = std::regex_replace(javascript_dict, fix_dictionary_names, "\"$1\" :");
    return json;
}
