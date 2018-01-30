// Copyright 2018 the Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMBINETERRAINPROFILER_H
#define COMBINETERRAINPROFILER_H

#include <string>

#include "quadtreepath.h"

/*
 * Singleton class for profiling terrain combine events. The programmer can use
 * this class to mark the beginning and end of events, and the timing of those
 * events will be written out to a log file. This class is intended for
 * performance debugging.
 */
class CombineTerrainProfiler {
  public:
    static CombineTerrainProfiler * instance() { return _instance; }
    
    inline void Begin(const std::string operation, const QuadtreePath & path, const size_t size) {
      log(BEGIN, operation, path.AsString(), size);
    }

    inline void End(const std::string operation, const QuadtreePath & path) {
      log(END, operation, path.AsString());
    }

  private:
    enum TerrainEvent { BEGIN, END };
    
    static const double NSEC_TO_SEC = 1e-9;
    static CombineTerrainProfiler * const _instance;
    
    CombineTerrainProfiler() {}
    void log(TerrainEvent event, const std::string operation, const std::string object, const size_t size = 0);
    double getTime() const;
};

/*
 * A convenience class for profiling a block of code. Profiling begins when an
 * instance of this class is created and ends when the instance goes out of
 * scope.
 */
class BlockProfiler {
  public:
    BlockProfiler(const std::string operation, const QuadtreePath & path, const size_t size) :
        operation(operation), path(path)
    {
      terrainProf->Begin(operation, path, size);
    }
    ~BlockProfiler() {
      terrainProf->End(operation, path);
    }
  private:
    static CombineTerrainProfiler * const terrainProf;
    const std::string operation;
    const QuadtreePath path;
};

// Programmers should use these functions to profile code rather than the
// functions above. This makes it easy to use compile time flags to exclude
// the profiling code.
#define BEGIN_TERRAIN_PROF(op, path, size) { BlockProfiler prof(op, path, size)
#define END_TERRAIN_PROF() }

#endif // COMBINETERRAINPROFILER_H