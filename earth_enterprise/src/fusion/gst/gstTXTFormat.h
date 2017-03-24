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

#ifndef KHSRC_FUSION_GST_GSTTXTFORMAT_H__
#define KHSRC_FUSION_GST_GSTTXTFORMAT_H__

#include <gstFormat.h>

class gstTXTTable;
class gstRegistry;

class gstTXTFormat : public gstFormat {
 public:
  gstTXTFormat(const char* n);
  ~gstTXTFormat();

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();

  virtual const char* FormatName() { return "generic text file"; }
 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(uint32 layer, uint32 id);
  virtual gstRecordHandle GetAttributeImpl(uint32 layer, uint32 id);
  FORWARD_GETFEATUREBOX_TO_BASE;

  bool DiscoverLayout(const char* fname, gstRegistry* inforeg);

  uint lat_field_;
  uint lon_field_;
  double lat_multiplier_;
  double lon_multiplier_;

  gstTXTTable* table_;
};

#endif  // !KHSRC_FUSION_GST_GSTTXTFORMAT_H__
