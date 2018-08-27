#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_JSONIFY_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_JSONIFY_H_

#include <fstream>
#include <cstdio>
#include <string>
#include <iostream>
#include <locale>
#include <exception>
#include <typeinfo>
#include <sstream>
#include <cstdint>
#include <cmath>
#include <list>
#include <iomanip>
#include <cstring>
#include <memory>
#include <array>

/**
 * The "JSON" provided by glc files is really a javascript statement.  Convert it to more valid JSON
 */
std::string jsonifyStatement(const std::string& in);

#endif // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_JSONIFY_H_
