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
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t numRecords;
    double xmin;
    double ymin;
    double xmax;
    double ymax;
  };

  struct FileHeaderV2 {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t numRecords;
    std::uint32_t primType;     // geometric primitive type for all records
    double xmin;
    double ymin;
    double xmax;
    double ymax;
  };

  struct RecordPos {
    std::int64_t offset;
    std::uint32_t size;
    std::uint32_t filler;
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
  gstGeodeHandle GetGeode(std::uint32_t id);
  gstStatus AddGeode(gstGeodeHandle g);

  gstBBox GetGeodeBox(std::uint32_t id);

  std::uint32_t NumRecords() const { return file_header_.numRecords; }

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
  gstKVGeomFormat(const gstKVGeomFormat&) = delete;
  gstKVGeomFormat(gstKVGeomFormat&&) = delete;
  gstKVGeomFormat& operator=(const gstKVGeomFormat&) = delete;
  gstKVGeomFormat& operator=(gstKVGeomFormat&&) = delete;

  // gstFormat
  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();
  virtual const char* FormatName() { return "Keyhole Vector Geometry Format"; }

 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstBBox GetFeatureBoxImpl(std::uint32_t layer, std::uint32_t fid);

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

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstBBox GetFeatureBoxImpl(std::uint32_t layer, std::uint32_t fid);

  bool OpenSource(int id);
  bool CheckOpenSource(int id);
  int DetermineSource(std::uint32_t fid, std::uint32_t* true_fid);

  std::vector<gstSource*> sources_;
  std::vector<std::uint32_t> feature_counts_;
  std::map<int, std::uint32_t> feature_map_;

  int opened_source_;
  int last_source_id_;
  std::uint32_t last_range_start_;
  std::uint32_t last_range_end_;
};

#endif  // !KHSRC_FUSION_GST_GSTKVPFILE_H__
