#include <iomanip>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

#include "common/notify.h"
#include "combineterrain.h"
#include "combineterrainprofiler.h"

using namespace std;

// Pointer to the singleton instance for speed and convenience
CombineTerrainProfiler * terrainProf = CombineTerrainProfiler::instance();

// Initialize static members
const string CombineTerrainProfiler::COMPRESS_OP = "compress";
const string CombineTerrainProfiler::COMBINE_OP = "combine";
const string CombineTerrainProfiler::READ_OP = "read";
const string CombineTerrainProfiler::WRITE_OP = "write";
CombineTerrainProfiler * CombineTerrainProfiler::_instance = NULL;

CombineTerrainProfiler * CombineTerrainProfiler::instance() {
  if (!_instance) {
    _instance = new CombineTerrainProfiler();
  }
  return _instance;
}

void CombineTerrainProfiler::log
    (TerrainEvent event, string operation, string object) {
  stringstream message;
  
  message.setf(ios_base::fixed, ios_base::floatfield);
  
  message << "Thread " << syscall(SYS_gettid) << ", Time "
          << setprecision(6) << getTime() << ": ";
  
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

double CombineTerrainProfiler::getTime() {
  struct timespec currTime;
  clock_gettime(CLOCK_MONOTONIC, &currTime);
  return static_cast<double>(currTime.tv_sec) + (currTime.tv_nsec * NSEC_TO_SEC);
}
