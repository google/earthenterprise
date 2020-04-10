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

#ifndef KHSRC_FUSION_GST_GSTKHMFORMAT_H__
#define KHSRC_FUSION_GST_GSTKHMFORMAT_H__

#include <gstSpatialRef.h>
#include <gstFormat.h>

#include <vector>

class khVirtualRaster;

class gstKHMFormat : public gstFormat {
 public:
  gstKHMFormat(const char* n);

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();

  virtual const char* FormatName() {return "Keyhole Mosaic Format";}

 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id);
  FORWARD_GETFEATUREBOX_TO_BASE;


  std::vector<gstBBox> extents_geometry_;
  std::vector<gstRecordHandle> extents_attribs_;

  std::vector<gstBBox> tiles_geometry_;
  std::vector<gstRecordHandle> tiles_attribs_;

  void PopulateTiles(const khVirtualRaster& vr, gstHeaderHandle& header);
};

#endif  // !KHSRC_FUSION_GST_GSTKHMFORMAT_H__
