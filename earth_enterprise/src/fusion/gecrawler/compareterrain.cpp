// Copyright 2017 Google Inc.
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


#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <packet.h>
#include <third_party/rsa_md5/crc32.h>
#include <notify.h>
#include <khEndian.h>
#include "compareterrain.h"

namespace {

const double kEpsilonXY = 1.0e-10;
const double kEpsilonZ = 1.0e-5;

const std::string kLinePrefix = "\n      ";

template<class T1,class T2,class T3> void FormatThree(std::ostream &strm,
                                             const T1 &v1,
                                             const T2 &v2,
                                             const T3 &v3) {
  strm << "(" << v1 << "," << v2 << "," << v3 << ")";
}

const size_t kEmptyMeshHeaderSize = 16;

// Structures for mesh vertices and faces

struct MeshVertex {
  unsigned int x;
  unsigned int y;
  float z;

  void Pull(EndianReadBuffer &buf) {
    unsigned char c_tmp;
    buf >> c_tmp;
    x = c_tmp;
    buf >> c_tmp;
    y = c_tmp;
    buf >> z;
  }
};

std::ostream &operator<<(std::ostream &strm, const MeshVertex &v) {
  FormatThree(strm, v.x, v.y, v.z);
  return strm;
}

struct MeshFace {
  uint16 a, b, c;

  void Pull(EndianReadBuffer &buf) {
    buf >> a >> b >> c;
  }
};

std::ostream &operator<<(std::ostream &strm, const MeshFace &f) {
  FormatThree(strm, f.a, f.b, f.c);
  return strm;
}

// Terrain mesh

class Mesh {
 public:
  Mesh() { Reset(); }
  ~Mesh() {}
  void Reset() {
    source_size_ = 0;
    ox_ = 0.0;
    oy_ = 0.0;
    dx_ = 0.0;
    dy_ = 0.0;
    num_points_ = 0;
    num_faces_ = 0;
    level_ = 0;
    faces_.clear();
    vertices_.clear();
  }
  inline int32 source_size() const { return source_size_; }
  inline double ox() const { return ox_; }
  inline double oy() const { return oy_; }
  inline double dx() const { return dx_; }
  inline double dy() const { return dy_; }
  inline int32 num_points() const { return num_points_; }
  inline int32 num_faces() const { return num_faces_; }
  inline int32 level() const { return level_; }
  inline const MeshVertex &Vertex(size_t i) { return vertices_.at(i); }
  inline const MeshFace &Face(size_t i) { return faces_.at(i); }

  void Pull(EndianReadBuffer &buf) {
    // If size is 0, this is an empty (16 byte) header
    buf >> source_size_;
    if (source_size_ != 0) {
      EndianReadBuffer::size_type start_pos = buf.CurrPos();
      buf >> ox_
          >> oy_
          >> dx_
          >> dy_
          >> num_points_
          >> num_faces_
          >> level_;
      vertices_.resize(num_points_);
      for (int i = 0; i < num_points_; ++i) {
        buf >> vertices_[i];
      }
      faces_.resize(num_faces_);
      for (int i = 0; i < num_faces_; ++i) {
        buf >> faces_[i];
      }
      if (buf.CurrPos() != start_pos + source_size_) {
        throw khSimpleException(
            "Terrain mesh header error, size doesn't match contents");
      }
    } else {
      buf.Skip(kEmptyMeshHeaderSize - sizeof(source_size_));
      Reset();
    }
  }

  void PrintHeader(std::ostream &strm) {
    strm << "[size=" << source_size()
         << " o=(" << ox() << "," << oy() << ")"
         << " d=(" << dx() << "," << dy() << ")"
         << " points=" << num_points()
         << " faces=" << num_faces()
         << " level=" << level() << "]";
  }

  void PrintMesh(std::ostream &strm) {
    static const std::string kMeshPrefix(kLinePrefix + " ");

    strm << kMeshPrefix;
    PrintHeader(strm);
    strm << kMeshPrefix << " Points:";
    for (size_t i = 0; i < vertices_.size(); ++i) {
      strm << kMeshPrefix << "   p[" << i << "] = " << vertices_[i];
    }
    strm << kMeshPrefix << " Faces:";
    for (size_t i = 0; i < faces_.size(); ++i) {
      strm << kMeshPrefix << "   f[" << i << "] = " << faces_[i];
    }
  }
 private:
  int32 source_size_;
  double ox_;
  double oy_;
  double dx_;
  double dy_;
  int32 num_points_;
  int32 num_faces_;
  int32 level_;
  std::vector<MeshVertex> vertices_;
  std::vector<MeshFace> faces_;
};

inline bool IsAlmostEqual(const double a, const double b, const double epsilon) {
  return (fabs(a-b) <= epsilon) ? true : false;
}

inline bool IsAlmostEqual(const MeshVertex &v1, const MeshVertex &v2) {
  return v1.x == v2.x && v1.y == v2.y && IsAlmostEqual(v1.z, v2.z, kEpsilonZ);
}

bool CompareMesh(int mesh_number,
                 EndianReadBuffer *buffer1,
                 EndianReadBuffer *buffer2,
                 const bool ignore_mesh,
                 std::ostream *snippet) {
  Mesh mesh1, mesh2;
  *buffer1 >> mesh1;
  *buffer2 >> mesh2;

  if (getNotifyLevel() >= NFY_VERBOSE) {
    std::ostringstream headers;
    headers << "Mesh " << mesh_number
            << " Terrain Headers:\n";
    mesh1.PrintHeader(headers);
    headers << "\n";
    mesh2.PrintHeader(headers);
    notify(NFY_VERBOSE, "%s", headers.str().c_str());
  }

  if (mesh1.source_size() == 0  &&  mesh2.source_size() == 0) {
    return true;                        // filler for missing mesh
  }

  if (!IsAlmostEqual(mesh1.ox(), mesh2.ox(), kEpsilonXY)
      || !IsAlmostEqual(mesh1.oy(), mesh2.oy(), kEpsilonXY)
      || !IsAlmostEqual(mesh1.dx(), mesh2.dx(), kEpsilonXY)
      || !IsAlmostEqual(mesh1.dy(), mesh2.dy(), kEpsilonXY)
      || mesh1.level() != mesh2.level()) {
    *snippet << kLinePrefix << "Mesh " << mesh_number << " header mismatch:"
             << kLinePrefix << "  ";
    mesh1.PrintHeader(*snippet);
    *snippet << kLinePrefix << "  ";
    mesh2.PrintHeader(*snippet);
    return false;
  }

  if (ignore_mesh)
    return true;

  if(mesh1.num_points() != mesh2.num_points()
     || mesh1.num_faces() != mesh2.num_faces()) {
    *snippet << kLinePrefix << "Mesh " << mesh_number
             << " header point/face count mismatch:"
             << kLinePrefix << "  ";
    mesh1.PrintHeader(*snippet);
    *snippet << kLinePrefix << "  ";
    mesh2.PrintHeader(*snippet);
    return false;
  }

  bool success = true;

  // Compare mesh points
  for (int i = 0; i < mesh1.num_points(); ++i) {
    if (!IsAlmostEqual(mesh1.Vertex(i), mesh2.Vertex(i))) {
      *snippet << kLinePrefix << "Mesh " << mesh_number
               << " point ["<< i << "] mismatch";
      success = false;
    }
  }

  // Compare mesh faces
  for (int i = 0; i < mesh1.num_faces(); ++i) {
    const MeshFace &f1 = mesh1.Face(i);
    const MeshFace &f2 = mesh2.Face(i);
    if ((f1.a != f2.a || f1.b != f2.b || f1.c != f2.c)) {
      *snippet << kLinePrefix << "Mesh " << mesh_number
               << " face [" << i << "] mismatch";
      success = false;
    }
  }

  if (!success) {
    *snippet << kLinePrefix << "Source 1 mesh " << mesh_number << ":";
    mesh1.PrintMesh(*snippet);
    *snippet << kLinePrefix << "Source 2 mesh " << mesh_number << ":";
    mesh2.PrintMesh(*snippet);
  }

  return success;
}

} // namespace

namespace gecrawler {

bool CompareTerrainPackets(EndianReadBuffer *buffer1,
                           EndianReadBuffer *buffer2,
                           const bool ignore_mesh,
                           std::ostream *snippet) {
  bool success = true;
  *snippet << std::setprecision(12) << std::dec;

  int mesh_number = 0;

  try {
    while (!buffer1->AtEnd() && !buffer2->AtEnd()) {
      if (!CompareMesh(mesh_number, buffer1, buffer2, ignore_mesh, snippet)) {
        success = false;
      }
      ++mesh_number;
    }

    if (success && !buffer1->AtEnd()) {
      *snippet << kLinePrefix
               << "Excess content at end of source 1 terrain packet, sizes=("
               << buffer1->size() << "," << buffer2->size() << ")";
      success = false;
    }
    if (success && !buffer2->AtEnd()) {
      *snippet << kLinePrefix
               << "Excess content at end of source 2 terrain packet, sizes=("
               << buffer1->size() << "," << buffer2->size() << ")";
      success = false;
    }
  }
  catch (khSimpleException ex) {
    *snippet << kLinePrefix << "ERROR: Caught exception comparing mesh "
             << mesh_number << ": " << ex.what();
    success = false;
  }

  return success;
}


} // namespace gecrawler
