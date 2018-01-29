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

    inline void BeginCombine(const QuadtreePath & path) {
      log(BEGIN, COMBINE_OP, path.AsString());
    }
    inline void EndCombine(const QuadtreePath & path) {
      log(END, COMBINE_OP, path.AsString());
    }

    inline void BeginRead(const QuadtreePath & path) {
      log(BEGIN, READ_OP, path.AsString());
    }
    inline void EndRead(const QuadtreePath & path) {
      log(END, READ_OP, path.AsString());
    }

    inline void BeginWrite(const QuadtreePath & path) {
      log(BEGIN, WRITE_OP, path.AsString());
    }
    inline void EndWrite(const QuadtreePath & path) {
      log(END, WRITE_OP, path.AsString());
    }

  private:
    static const std::string COMPRESS_OP;
    static const std::string COMBINE_OP;
    static const std::string READ_OP;
    static const std::string WRITE_OP;
    
    enum TerrainEvent { BEGIN, END };
    
    static const double NSEC_TO_SEC = 1e-9;
    
    static CombineTerrainProfiler * _instance;
    CombineTerrainProfiler() {}
    void log(TerrainEvent event, std::string operation, std::string object);
    double getTime();
};

// Pointer to the singleton instance for speed and convenience
extern CombineTerrainProfiler * terrainProf;

#endif // COMBINETERRAINPROFILER_H