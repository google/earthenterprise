/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _PROCMESH_H
#define _PROCMESH_H

#include "mesh.h"
#include "quadric.h"
#include "procmesh.h"
#include "heap.h"
#include "array.h"

class etQemTriangle;
class etQemPoint;

class etQemInfo {
 public:
  // Decimation parameters
  double  borderTargetError; // Target error for the border decimation
  double  borderWeight;      // Border quadric weight
  double  targetError;       // Target approximation error
  double  minError;          // Minimum QEM error
  double  triangleQuality;   // Minimum triangle quality

  int     minTriangles;      // Target number of triangles
  int     sizeX;             // Tile size
  int     sizeY;             // Tile size
  //    int     weightQuality;     // Whether or not triangle quality
  //                               // weighting should be used.
  void reset()
  {
    targetError       = 1.0E9;
    borderTargetError = 1.0E9;
    minTriangles      = 2;
    borderWeight      = 1000.0;
    triangleQuality   = 1E-2;
    sizeX             = 0;
    sizeY             = 0;
    //        weightQuality     = true;
  }
};

//////////////////////////////////////////////////////////////////////////////
// Definition of a Quadric Error Metric vertex.
//
// Each vertex has:
// - A quadric
// - A candidate to which is should collapse with if it's selected.
// - A quadric error metric value
// - A geometric deviation value.
// - The list of surrounding triangles.
//
// TODO: Rethink the way I collect the surrounding triangles. With meshes with
//       holes like the bunny, I can end up with multiple isolated triangles
//       connected to a given vertex. Then with the current method, it's not
//       possible to find the list of surrounding triangles.
//
// - A weight. It will be used to compute the quadrics per vertex. With this
//   method, it will be possible to give more importance to certain points.
//
//////////////////////////////////////////////////////////////////////////////
class etQemPoint : public etHeapable {
 private:


  // Definition of a potential candidate.
  class etQemCandidate {
   public:
    double      error; // The cost to collapse with that vertex.
    etQemPoint *point; // Vertex that will remain after the collapse
    etQemCandidate() { reset(); }
    void reset() { point = 0; error = 0.0; }
  };

  enum {
    CONSTRAINED     = 0x00010000,     // Whether the point is constrained or not.
    BORDER          = 0x00020000,     // Whether the point is a border or not.
    COLLAPSED       = 0x00040000,     // Whether the point is collapsed or not.
    REMAINING_FLAGS = 0xfff80000,     // List of remaining bit unused.
    LEVEL_MASK      = 0x0000ffff,     // Group tag (currently unused).
  };

  etQVector<double,3>     v;            // values
  etQuadric<double,4>     quadric;      // associated quadric
  double                  deviation;    // Quadric deviation
  double                  weight;       // Vertex weight; (currently unused)
  etQemCandidate          candidate;    // Best candidate to merge this point with
  unsigned int            flags;        // Status of the point
  etArray<etQemTriangle *> poly;        // List of surrounding triangles
 public:
  // Constructors
  etQemPoint() { init(); }
  etQemPoint(const etVec3f &p) { init(); set(p); }

  // operators
  double& operator[](int index) { return v[index]; }
  const double operator[](int index) const { return v[index]; }


  // inlines.
  inline etQuadric<double,4> &getQuadric() { return quadric; }
  void   setQuadric(const etQuadric<double,4> &q) { quadric = q; }

  inline int  getLevel() const { return flags & LEVEL_MASK; }
  inline void setLevel(const int lvl) {
    flags &= ~LEVEL_MASK;
    flags |= (lvl & LEVEL_MASK);
  }

  // To know whether a vertex is collapsed or not.
  // Note: The set method is protected.
  inline bool isCollapsed()  const { return (flags & COLLAPSED) == COLLAPSED; }

  // To know whether the vertex is a border vertex or not
  // If so, I can only be collapsed with another border vertex.
  inline bool isBorder()  const { return (flags & BORDER) == BORDER; }
  inline void setBorder() { flags |= BORDER; }

  // To know whether the vertex is constrained or not.
  // If so, it cannot be ´collapsed.
  inline bool isConstrained()  const { return (flags & CONSTRAINED) == CONSTRAINED; }
  inline void setConstrained() { flags |= CONSTRAINED; }

  inline void setWeight(const double w) { weight = w; }
  inline double getWeight() const { return weight; }

  // To get the actual data from the vertex.
  inline etQVector<double,3> &getVector() { return v; }

  // To get the computed quadric error metric.
  inline double      getQem()   const { return candidate.error; }

  // To get the candidate where to collapse the vertex to.
  inline etQemPoint *getPoint() const { return candidate.point; }

  // To reset the possible candidate.
  inline void resetCandidate() {  candidate.reset(); deviation = 0.0; }

  // To add a quadric to the vertex.
  inline void addQuadric(const etQuadric<double,4> &q) { quadric += q; }

  // Get the geometric deviation from the quadric error metric.
  inline double getDeviation() const { return deviation; }

  // Assign a vector to this point.
  inline void set(const etVec3f &p)
  {
    v[0] = p[0];
    v[1] = p[1];
    v[2] = p[2];
  }

  inline void get(etVec3f &p)
  {
    p[0] = (float) v[0];
    p[1] = (float) v[1];
    p[2] = (float) v[2];
  }

  inline void getd(etVec3d &p)
  {
    p[0] = v[0];
    p[1] = v[1];
    p[2] = v[2];
  }

  inline void getxydoubles(double *p)
  {
    p[0] = v[0];
    p[1] = v[1];
  }


  // This function will recompute the qem of a certain point.
  void updateQem2D(etQemInfo *ctx);
  void updateQem3D(etQemInfo *ctx);

  // This function compute a specific deviation error for border
  // vertices.
  void updateBorderError();

  // This function will merge the vertex with its candidate.
  // The function returns the number of triangles merged due to the point
  // collapse and the list of affected vertices that need a qem recompute
  int merge(etArray<etQemPoint*> &ppt);
  int mergeBorder(etArray<etQemPoint*> &ppt);

  // Function returning true if the merge between this vertex and
  // the best candidate won't create a fold.
  bool canBeMerged();

  // This function returns true if the vertex is not a collapsed one.
  bool isValid() const { return !isCollapsed(); }

  // Function requested by the heap for sorting.
  // XXX - someday we need to change etHeap to be a template around
  // the type of object it's containing. This will eliminate the
  // need for a virtual.
  virtual double getKey() { return candidate.error; }

  void addTriangle(etQemTriangle *ptr) { poly.add(&ptr); }

  // Function used to collect the worst triangle quality ratio
  // if a given vertex was merged with another one.
  // I am using the same formula as the one suggested by Paul Heckbert
  // in his Quadric Error Metric decimation document.
  double old_getQualityRatio2D(etQemPoint *can);
  double getQualityRatio2D(etQemPoint *can);
  double getQualityRatio3D(etQemPoint *can);

  void cleanUp();

  inline void setCollapsed() { flags |= COLLAPSED; }

 protected:

  // This function will compute the qem for the current vertex with
  // the quadric of pt. If the error is smaller than the existing
  // candidate, then the candidate is selected.
  void possibleCandidate2D(etQemInfo *ctx, etQemPoint *pt);
  void possibleCandidate3D(etQemInfo *ctx, etQemPoint *pt);

  void init() { flags = 0x0L; deviation = 0.0; weight = 1.0; poly.setChunck(10); }
};


//////////////////////////////////////////////////////////////////////////////
// Definition of a triangle.
//
// Each of them must have:
// - The oriented list of vertices that describe the triangle
// - The oriented list of adjacent triangles
// - The information whether the triangle is collapsed or not.
// Note: The vertices & adjacent triangles are oriented CCW.
//
//////////////////////////////////////////////////////////////////////////////
class etQemTriangle {
  enum {
    COLLAPSED       = 0x00010000,
  };
 public:
  etQemPoint    *verts[3];
  etQemTriangle *neighbours[3];
  unsigned int   flags;

  etQemTriangle() { init(); }
  etQemTriangle(etQemPoint *p1, etQemPoint *p2, etQemPoint *p3) {
    init(); setPoints(p1, p2, p3);
  }

  void setPoints(etQemPoint *p1, etQemPoint *p2, etQemPoint *p3);
  void setNeighbours(etQemTriangle *n1, etQemTriangle *n2, etQemTriangle *n3);
  etQemPoint    *getPoint(const int idx) const { return verts[idx]; }
  etQemTriangle *getNeighbour(const int idx) { return neighbours[idx]; }

  // Function responsible for computing the quadric and spreading it
  // to the vertices
  void buildQuadric();

  inline bool isCollapsed()  const { return (flags & COLLAPSED) == COLLAPSED; }
  inline void setCollapsed() { flags |= COLLAPSED; }

  bool isValid() const { return !isCollapsed(); }

 protected:
  void init() { verts[0] = verts[1] = verts[2] = 0;
    neighbours[0] = neighbours[1] = neighbours[2] = 0;
    flags = 0; }
};

//////////////////////////////////////////////////////////////////////////////
// Definition of the mesh used for the quadric error metrics processing.
//
// It must contain:
// - The list of vertices describing the mesh
// - The list of connected triangles describing the mesh.
// - A priority list.
//
// IMPORTANT: The object can only supports manifold meshes. If you wish to
//            process non manifold meshes, you will have to convert them to
//            manifold meshes first.
//
//////////////////////////////////////////////////////////////////////////////
class etQemMesh
{
  etArray<etQemPoint>    points;        // List of vertices
  etArray<etQemTriangle> triangles;     // List of triangles
  etHeap                 priolist;      // Priority list constaining the sorted vertices
  int                    pointsLeft;    // Number of vertices left in the mesh.
  int                    trianglesLeft; // Number of triangles left in the mesh.

  etQemInfo              info;          // Structure containing decimation parameters.

 public:
  // Constructors
  etQemMesh( etVec3f  *pts, int npts, etFace3i *tri, int ntri,
             etFace3i *neigh = 0);

  // In this version, I give also the normal per vertex needed to compute
  // the quadric.
  etQemMesh( etVec3f  *pts, int npts, etVec3f *normals,
             etFace3i *tri, int ntri, etFace3i *neigh = 0);

  // Destructor
  ~etQemMesh();


  // Note: The following inlines must be set prior to decimating the mesh.
  // Defines the minimum geometric error accepted.
  inline void setTargetError(const double v) { info.targetError = v;    }
  inline double getTargetError()       const { return info.targetError; }

  // Defines the minimum geometric error accepted for the borders.
  inline void setBorderTargetError(const double v) { info.borderTargetError = v;    }
  inline double getBorderTargetError()       const { return info.borderTargetError; }


  // Defines the minimum number of triangles accepted.
  inline void setMinTriangles(const int v) { info.minTriangles = v;    }
  inline int getMinTriangles()       const { return info.minTriangles; }

  // Defines the penalty weight to apply to border vertices.
  inline void setBorderWeight(const double v) { info.borderWeight = v;    }
  inline double getBorderWeight()       const { return info.borderWeight; }

  // Defines the minimum triangle quality for the mesh.
  inline void setTriangleQuality(const double q) { info.triangleQuality = q;    }
  inline double getTriangleQuality()       const { return info.triangleQuality; }

  void FixDanglingPoints(void);

  // Defines the minimum QEM error possible. Usefull for flat surfaces.
  inline void setMinError(const double v) { info.minError = v;    }
  inline double getMinError()       const { return info.minError; }

  inline void setSizeX(const int sz) { info.sizeX = sz; }
  inline int  getSizeX() const { return info.sizeX;     }

  inline void setSizeY(const int sz) { info.sizeY = sz; }
  inline int  getSizeY() const { return info.sizeY;     }

  //    inline void setWeightQuality(const int v) { info.weightQuality = v; }
  //    inline int getWeightQuality() const { return info.weightQuality;    }

  // This method decimates a generic mesh to a given value.
  void decimate();

  // This method specifically decimates terrain tiles.
  // The function may accept as parameter the normals per vertex used
  // to define the quadrics. This should ensure that border vertices
  // have the same quadrics whatever the tile.
  void decimateTile();

  // Returns the number of points & triangles left after the decimation
  // process.
  inline int getNumPointsLeft()    const { return pointsLeft;    }
  inline int getNumTrianglesLeft() const { return trianglesLeft; }


  // Function used to generate a triangle mesh with the remains
  // of the mesh decimation.
  void getMesh(etVec3f  *pts, int &npts, etFace3i *tri, int &ntri,
               etFace3i *neigh);
  void getMesh(etMesh<etVec3f, etFace3i> &msh);
  void getTile(etArray<etVec3f> &pts, etArray<etFace3i> &fcs, etVec3f *verts);

  void optimizeIndices(etArray<etVec3f> &pts, etArray<etFace3i> &fcs);

 protected:

  // This method will generate the quadrics for all vertices.
  void computeQuadrics(etVec3f *normals = 0);

  // This method identifies all the border vertices of a mesh
  // if any.
  // Note: Works only with manifold meshes.
  void tagBorderVertices();

  // This function will process each vertex, estimate is best collapse
  // vertex.
  void findCandidates3D();
  void findCandidates2D();

  // This function will initialize the priority list and insert the vertices.
  void sortVertices(bool bordersOnly = false);

  // This method will process all the edges and feed the priority list.
  // If the mesh is not connected, I will do it.
  void processBorderVertices(etVec3f *normal = 0);

  // Function used to collapse a mesh until it reaches a
  // targetted number of triangles.
  int collapse2D();
  int collapse3D();

  // Function used to collapse borders. It is specifically designed for
  // the terrain. Only an error limit can be specified to stop it.
  int collapseBorders();

  // Internal method to compute a border quadric
  void borderQuadric(const etQVector<double, 3> &n, etQemPoint *p1,
                     etQemPoint *p2);

  // Responsible for constraining all border vertices so that they cannot
  // be deleted by another decimation process.
  void constrainBorderVertices();

  static int compareTriInfo( const void *arg1, const void *arg2 );
};


#endif
