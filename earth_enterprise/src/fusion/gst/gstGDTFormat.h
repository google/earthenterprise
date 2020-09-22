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

#ifndef KHSRC_FUSION_GST_GSTGDTFORMAT_H__
#define KHSRC_FUSION_GST_GSTGDTFORMAT_H__

#include <gstFormat.h>

class gstTXTTable;
class gstFileInfo;

class gstGDTFormat : public gstFormat {
 public:
  enum GDTType {
    TransType1_v3,
    TransType1_v5,
    TransType2_v3,
    TransType2_v5,
    TransType1Hwy,
    TransType7,
    PostalCodeInventory,
    PostalCodeBoundary,
    CountyInventory,
    CountyBoundary,
    PlaceInventory,
    PlaceBoundary,
    DynamapPrimary,
    Unknown
  };

  gstGDTFormat(const char* n);
  ~gstGDTFormat();

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();
  virtual const char* FormatName() { return "GDT Dynamap/Transportation File"; }
 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id);
  FORWARD_GETFEATUREBOX_TO_BASE;


  bool Parse();

  std::vector<gstRawGeode> raw_geometry_;
  std::vector<gstRecordHandle> attributes_;

  struct GeoField {
    int x;
    int y;
    GeoField(int ix, int iy) : x(ix), y(iy) { }
  };

  struct FileDef {
    gstPrimType primType;
    gstTXTTable* table;
    std::vector<GeoField> geoFields;
    FileDef() : table(NULL) {}
  };

  void Configure(FileDef& fd, GDTType t, const std::string& file_name);

  FileDef primary_def_;
  FileDef secondary_def_;
};

#endif  // !KHSRC_FUSION_GST_GSTGDTFORMAT_H__
