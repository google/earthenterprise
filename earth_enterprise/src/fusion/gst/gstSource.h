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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSOURCE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSOURCE_H_

#include "fusion/gst/gstFileUtils.h"
#include "fusion/gst/gstTypes.h"
#include "fusion/gst/gstMemory.h"
#include "fusion/gst/gstBBox.h"
#include "fusion/gst/gstFormat.h"
#include "fusion/gst/gstGeode.h"

class gstHeader;
class gstSourceManager;

class gstSource : public gstMemory {
 public:
  explicit gstSource(const QString &);
  ~gstSource();

  gstStatus Status() const { return status_; }

  gstStatus Open();

  void Close();

  bool Opened() const { return opened_; }

  const QString& Codec() const { return codec_; }
  bool SetCodec(const QString &c);

  void SetEnabled(bool e);

  // retrieve contents of file through the format
  gstFormat* Format() const { return format_; }

  // convenience routines for format object
  uint32 NumLayers() const;
  uint32 NumFeatures(uint32 layer) const;
  double AverageFeatureDiameter(uint32 layer) const;
  uint32 RecommendedMaxResolutionLevel() const;
  uint32 RecommendedMinResolutionLevel() const;
  uint32 RecommendedEfficientResolutionLevel() const;

  // ===== new sequential access API =====
  void ResetReadingOrThrow(uint32 layer);
  gstGeodeHandle GetNormCurrFeatureOrThrow(void);
  gstRecordHandle GetCurrentAttributeOrThrow(void);
  void IncrementReadingOrThrow(void);
  bool IsReadingDone(void);

  // ===== random access API =====
  gstGeodeHandle GetFeatureOrThrow(uint32 layer, uint32 id,
                                   bool is_mercator_preview);
  gstRecordHandle GetAttributeOrThrow(uint32 layer, uint32 id);
  gstBBox GetFeatureBoxOrThrow(uint32 layer, uint32 id);


  gstPrimType GetPrimType(uint32 layer) const;
  const gstHeaderHandle& GetAttrDefs(uint32 layer) const;
  bool HasAttrib(uint32 layer) const {
    return (GetAttrDefs(layer)->numColumns() > 0);
  }
#if 0
  gstBBox BoundingBox();
#endif
  gstBBox BoundingBox(uint32 layer);

  bool NoFile() const { return no_file_; }
  void SetNoFile(bool nf) { no_file_ = nf; }

  void DefineSrs(const QString& wkt, bool savePrj);
  void SavePrj(const QString& wkt);

 private:
  friend class gstSourceManager;

  std::vector<gstLayerDef>& LayerDefs() const;

  bool opened_;

  gstFormat* format_;
  gstStatus status_;

  QString codec_;

  mutable std::vector<gstLayerDef> layer_defs_;

  bool no_file_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSOURCE_H_
