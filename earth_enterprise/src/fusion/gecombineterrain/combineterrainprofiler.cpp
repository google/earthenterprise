#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "common/notify.h"
#include "combineterrain.h"
#include "combineterrainprofiler.h"

using namespace std;

// Pointer to the singleton instance for speed and convenience
CombineTerrainProfiler * terrainProf = CombineTerrainProfiler::instance();

// Initialize static members
const string CombineTerrainProfiler::COMPRESS_OP = "compress";
CombineTerrainProfiler * CombineTerrainProfiler::_instance = NULL;

CombineTerrainProfiler * CombineTerrainProfiler::instance() {
  if (!_instance) {
    _instance = new CombineTerrainProfiler();
  }
  return _instance;
}

void CombineTerrainProfiler::log
    (TerrainEvent event, std::string operation, std::string object) {
  stringstream message;
  
  message << "Thread " << syscall(SYS_gettid) << ": ";
  
  switch(event) {
    case BEGIN:
      message << "Begin";
      break;
    case END:
      message << "End";
      break;
  }
  
  message << " " << operation << " " << object;
  
  notify(NFY_NOTICE, "%s\n", message.str().c_str());  
}
