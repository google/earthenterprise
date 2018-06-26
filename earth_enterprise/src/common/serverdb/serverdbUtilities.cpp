// Copyright 2018 Open GEE Contributors
#include "common/serverdb/serverdbUtilities.h"
#include <string>
#include <iostream>
#include <regex.h>

std::string ServerdbUtilities::ParseValidJSON(const std::string& javascript_dict) {
    //const regex_t *__restrict __preg,
    //const char *__restrict __string,
    // size_t __nmatch,
    //regmatch_t __pmatch[__restrict_arr],
    //int __eflags);
    int max_matches = 30;
    regmatch_t matches[max_matches];
    regex_t regex;
    regcomp(&regex, "([A-Za-z]+)\\s:", REG_EXTENDED);
    std::cout << "TESTIN!!!!!! " << std::endl;

    int match_successful= regexec(&regex, javascript_dict.data(), max_matches, matches, 0);
    if (match_successful) {
        for(int i = 0; i < max_matches; ++i) {
            regmatch_t match = matches[i];
            int index = match.rm_so;
            int offset = match.rm_eo;
            std::cout << "Index: " << index << std::endl;
            std::cout << "Offset: " << offset << std::endl;
        }
    }

    //std::string json = std::regex_replace(javascript_dict, fix_dictionary_names, "\"$1\" :");

    regfree(&regex);
    return "";
}

