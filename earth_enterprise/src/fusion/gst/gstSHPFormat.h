/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef KHSRC_FUSION_GST_GSTSHPFORMAT_H__
#define KHSRC_FUSION_GST_GSTSHPFORMAT_H__

#include <assert.h>

#include <gstFormat.h>

template <class Type>
inline void gstByteSwap(Type& in) {
  Type outgoing;
  char* ip = (char*)&in;
  char* op = &((char*)&outgoing)[sizeof(Type)-1];
  for (; op >= (char*)&outgoing; op--, ip++)
    *op = *ip;
  in = outgoing;
}

struct SHP_FileHeader_i;
struct SHP_RecHeader_i;
struct SHX_RecHeader_i;

class gstDBFTable;

class gstSHPFormat : public gstFormat {
 public:
  gstSHPFormat(const char* n);
  ~gstSHPFormat();

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();

  virtual gstGeode* GetFeature(std::uint32_t, std::uint32_t);
  virtual gstRecordHandle GetAttribute(std::uint32_t, std::uint32_t);

  virtual const char* FormatName() { return "ESRI SHP Shapefile"; }

 private:
  typedef gstGeode *(gstSHPFormat::*ShapeParseFunc)(unsigned char *a);
  gstGeode* parsePoint(unsigned char *a);
  gstGeode* parsePolyLine(unsigned char *a);
  gstGeode* parsePolygon(unsigned char *a);

  SHP_FileHeader_i* readFileHeader(FILE *f);
  SHP_RecHeader_i* readRecHeader(FILE *f);

  bool _bigEndian;
  unsigned int _numRecords;

  int _subparts;

  gstDBFTable* _table;

  SHP_FileHeader_i* _hdrSHP;
  FILE* _fpSHP;

  ShapeParseFunc _shapeParseFunc;

  SHX_RecHeader_i* _shxRecords;
};


// Integer (I): Signed 32-bit (4 bytes)
// Double  (D):  Signed 64-bit (8 bytes)
// Big Endian (big) or Little Endian (little)
enum {
  SHP_FILE_CODE,        // I - Big
  SHP_FILE_LENGTH,      // I - Big
  SHP_VERSION,          // I - Little
  SHP_SHAPE_TYPE,       // I - Little
  SHP_BBOX_XMIN,        // D - Little
  SHP_BBOX_YMIN,        // D - Little
  SHP_BBOX_XMAX,        // D - Little
  SHP_BBOX_YMAX,        // D - Little
  SHP_BBOX_ZMIN,        // D - Little
  SHP_BBOX_ZMAX,        // D - Little
  SHP_BBOX_MMIN,        // D - Little
  SHP_BBOX_MMAX // D - Little
};

// All the non-Null shapes in a shapefile are required to be of the
// same shape type.  The values for shape type are as follows:

enum {
  SHP_Null_Shape  = 0,
  SHP_Point       = 1,
  SHP_PolyLine    = 3,
  SHP_Polygon     = 5,
  SHP_MultiPoint  = 8,
  SHP_PointZ      = 11,
  SHP_PolyLineZ   = 13,
  SHP_PolygonZ    = 15,
  SHP_MultiPointZ = 18,
  SHP_PointM      = 21,
  SHP_PolyLineM   = 23,
  SHP_PolygonM    = 25,
  SHP_MultiPointM = 28,
  SHP_MultiPath   = 31
};

struct SHP_FileHeader_i {
  std::int32_t FileCode;               // Big Endian
  std::int32_t Unused0;                // Big Endian
  std::int32_t Unused1;                // Big Endian
  std::int32_t Unused2;                // Big Endian
  std::int32_t Unused3;                // Big Endian
  std::int32_t Unused4;                // Big Endian
  std::int32_t FileLength;             // Big Endian
  std::int32_t Version;                // Little Endian
  std::int32_t ShapeType;              // Little Endian
  double BBox_Xmin;             // Little Endian
  double BBox_Ymin;             // Little Endian
  double BBox_Xmax;             // Little Endian
  double BBox_Ymax;             // Little Endian
  double BBox_Zmin;             // Little Endian
  double BBox_Zmax;             // Little Endian
  double BBox_Mmin;             // Little Endian
  double BBox_Mmax;             // Little Endian
};

enum {
  SHP_RecordNum,
  SHP_ContentLength
};

struct SHX_RecHeader_i {
  std::int32_t Offset;         // Big Endian
  std::int32_t RecSize;                // Big Endian
};

struct SHP_RecHeader_i {
  std::int32_t RecordNumber;           // Big Endian
  std::int32_t ContentLength;          // Big Endian
};

struct SHP_Point_Rec {
  double x;                     // Little Endian
  double y;                     // Little Endian
};

struct SHP_MultiPoint_Rec {
  double Box[4];                // Xmin, Ymin, Xmax, Ymax
  std::int32_t NumPoints;
};

struct SHP_PolyLine_Rec {
  double Box[4];
  std::int32_t NumParts;               // Number of Parts
  std::int32_t NumPoints;              // Total number of Points
  std::int32_t *Parts;         // Index to First Point in Part
  SHP_Point_Rec *Points;        // Points for All Parts
};

struct SHP_Polygon_Rec {
  double Box[4];
  std::int32_t NumParts;               // Number of Parts
  std::int32_t NumPoints;              // Total number of Points
  std::int32_t *Parts;         // Index to First Point in Part
  SHP_Point_Rec *Points;        // Points for All Parts
};

struct SHP_PolygonZ_Rec {
  double Box[4];
  std::int32_t NumParts;               // Number of Parts
  std::int32_t NumPoints;              // Total number of Points
  std::int32_t *Parts;         // Index to First Point in Part
  SHP_Point_Rec *Points;        // Points for All Parts
  double *ZRange;
  double *ZNumPoints;
  double *MRange;
  double *MNumPoints;
};

#endif  // !KHSRC_FUSION_GST_GSTSHPFORMAT_H__
