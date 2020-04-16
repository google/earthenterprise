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

#ifndef KHSRC_FUSION_GST_GSTGDALFORMAT_H__
#define KHSRC_FUSION_GST_GSTGDALFORMAT_H__

#include <gstFormat.h>

class GDALDataset;

class gstGDALFormat : public gstFormat {
 public:
  gstGDALFormat(const char *n);
  ~gstGDALFormat();

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile() { return GST_OKAY; }
  virtual const char *FormatName() { return "GDAL Supported formats"; }

 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id);
  FORWARD_GETFEATUREBOX_TO_BASE;


  gstBBox coord_box_;
};

#endif  // !KHSRC_FUSION_GST_GSTGDALFORMAT_H__
