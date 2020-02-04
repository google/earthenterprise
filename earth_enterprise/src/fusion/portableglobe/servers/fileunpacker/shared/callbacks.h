
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_CALL_BACKS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_CALL_BACKS_H_

#include <functional>

class IndexItem;

using map_packet_walker = std::function<bool(int layer, const IndexItem&)>;

using map_file_walker = std::function<bool(int layer, const char*)>;

#endif // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_CALL_BACKS_H_
