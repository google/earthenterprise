// Copyright 2018 Open GEE Contributors

#ifndef EARTHENTERPRISE_SERVERDBUTILITIES_H
#define EARTHENTERPRISE_SERVERDBUTILITIES_H

#include <string>

class ServerdbUtilities {

public:

    /*
     * Takes a javascript dictionary and forms valid JSON from it.
     */
    std::string ParseValidJSON(const std::string& javascript_dict);

};

#endif //EARTHENTERPRISE_SERVERDBUTILITIES_H
