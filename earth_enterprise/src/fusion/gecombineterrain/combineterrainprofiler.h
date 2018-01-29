#ifndef COMBINETERRAINPROFILER_H
#define COMBINETERRAINPROFILER_H

#include <string>

#include "quadtreepath.h"

class CombineTerrainProfiler
{
  public:
    static CombineTerrainProfiler * instance();

    inline void BeginCompress(const QuadtreePath & path) {
      log(BEGIN, COMPRESS_OP, path.AsString());
    }
    inline void EndCompress(const QuadtreePath & path) {
      log(END, COMPRESS_OP, path.AsString());
    }

  private:
    static const std::string COMPRESS_OP;
    
    enum TerrainEvent { BEGIN, END };
    
    static CombineTerrainProfiler * _instance;
    CombineTerrainProfiler() {}
    void log(TerrainEvent event, std::string operation, std::string object);
};

// Pointer to the singleton instance for speed and convenience
extern CombineTerrainProfiler * terrainProf;

#endif // COMBINETERRAINPROFILER_H