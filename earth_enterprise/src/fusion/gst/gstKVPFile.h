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

#ifndef KHSRC_FUSION_GST_GSTKVPFILE_H__
#define KHSRC_FUSION_GST_GSTKVPFILE_H__

#include <gstMemory.h>
#include <notify.h>
#include <khArray.h>
#include <gstFormat.h>
#include <khGuard.h>
#include <map>

class gstHeader;
class gstSource;

class gstKVPTable;

#define KVP_MAGIC 0xab0120cd
#define KVP_V1 1

class gstKVPFile : public gstMemory {
 public:
  struct FileHeaderV1 {
    uint32 magic;
    uint32 version;
    uint32 numRecords;
    double xmin;
    double ymin;
    double xmax;
    double ymax;
  };

  struct FileHeaderV2 {
    uint32 magic;
    uint32 version;
    uint32 numRecords;
    uint32 primType;     // geometric primitive type for all records
    double xmin;
    double ymin;
    double xmax;
    double ymax;
  };

  struct RecordPos {
    int64 offset;
    uint32 size;
    uint32 filler;
    double xmin;
    double ymin;
    double xmax;
    double ymax;
  };


  gstKVPFile(const char* n);
  ~gstKVPFile();

 private:
  gstStatus Open(gstReadMode mode);
 public:
  gstStatus OpenForWrite() { return Open(GST_WRITEONLY); }
  gstStatus OpenForRead() { return Open(GST_READONLY); }
  gstStatus Close();

  // geode conversion routines
  gstGeodeHandle GetGeode(uint32 id);
  gstStatus AddGeode(gstGeodeHandle g);

  gstBBox GetGeodeBox(uint32 id);

  uint32 NumRecords() const { return file_header_.numRecords; }

  const gstBBox& BoundingBox() const { return bounding_box_; }

  RecordPos Allocate(gstGeodeHandle);

 private:
  gstStatus ReadHeader();
  bool ReadIndex();
  gstStatus Flush();

  // raw buffer to be used by addGeode() and getGeode()
  // when converting to/from geode/raw
  std::vector<char> rawBuf;

  int file_descriptor_;
  gstReadMode mode_;

  FileHeaderV2 file_header_;

  bool modified_;

  khArray<RecordPos> index_map_;

  gstBBox bounding_box_;

  off64_t current_offset_;
};

// -----------------------------------------------------------------------------

class gstKVGeomFormat : public gstFormat {
 public:
  gstKVGeomFormat(const char* n);
  virtual ~gstKVGeomFormat();

  // gstFormat
  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();
  virtual const char* FormatName() { return "Keyhole Vector Geometry Format"; }

 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(uint32 layer, uint32 id);
  virtual gstRecordHandle GetAttributeImpl(uint32 layer, uint32 id);
  virtual gstBBox GetFeatureBoxImpl(uint32 layer, uint32 fid);

 private:
  gstKVPFile* kvp_file_;
  gstKVPTable* kvp_table_;
};

// -----------------------------------------------------------------------------

class gstKVPFormat : public gstFormat {
 public:
  gstKVPFormat(const char* n);
  virtual ~gstKVPFormat();

  // gstFormat
  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();
  virtual const char* FormatName() { return "Keyhole Vector Product Format"; }

 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(uint32 layer, uint32 id);
  virtual gstRecordHandle GetAttributeImpl(uint32 layer, uint32 id);
  virtual gstBBox GetFeatureBoxImpl(uint32 layer, uint32 fid);

  bool OpenSource(int id);
  bool CheckOpenSource(int id);
  int DetermineSource(uint32 fid, uint32* true_fid);

  std::vector<gstSource*> sources_;
  std::vector<uint32> feature_counts_;
  std::map<int, uint32> feature_map_;

  int opened_source_;
  int last_source_id_;
  uint32 last_range_start_;
  uint32 last_range_end_;
};

#endif  // !KHSRC_FUSION_GST_GSTKVPFILE_H__
