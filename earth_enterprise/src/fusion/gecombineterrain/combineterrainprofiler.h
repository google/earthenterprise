#ifndef COMBINETERRAINPROFILER_H
#define COMBINETERRAINPROFILER_H

#include <string>

#include "quadtreepath.h"

class CombineTerrainProfiler
{
  public:
    static CombineTerrainProfiler * instance() { return _instance; }
    
    inline void Begin(const std::string operation, const QuadtreePath & path) {
      log(BEGIN, operation, path.AsString());
    }

    inline void End(const std::string operation, const QuadtreePath & path) {
      log(END, operation, path.AsString());
    }

  private:
    enum TerrainEvent { BEGIN, END };
    
    static const double NSEC_TO_SEC = 1e-9;
    static CombineTerrainProfiler * const _instance;
    
    CombineTerrainProfiler() {}
    void log(TerrainEvent event, const std::string operation, const std::string object);
    double getTime() const;
};

class BlockProfiler
{
  public:
    BlockProfiler(const std::string operation, const QuadtreePath & path) :
        operation(operation), path(path)
    {
      terrainProf->Begin(operation, path);
    }
    ~BlockProfiler() {
      terrainProf->End(operation, path);
    }
  private:
    static CombineTerrainProfiler * const terrainProf;
    const std::string operation;
    const QuadtreePath path;
};

#define BEGIN_TERRAIN_PROF(op, path) { BlockProfiler prof(op, path)
#define END_TERRAIN_PROF() }

#endif // COMBINETERRAINPROFILER_H