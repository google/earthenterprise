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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTOGRFORMAT_H__
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTOGRFORMAT_H__

#include <vector>

#include <ogrsf_frmts.h>

#include "fusion/gst/gstFormat.h"


class gstOGRFormat : public gstFormat {
 public:
  explicit gstOGRFormat(const char *n);
  ~gstOGRFormat();

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();

  virtual const char* FormatName() { return "OGR Supported formats"; }

 private:
  virtual void ResetReadingImpl(std::uint32_t layer);
  virtual gstGeodeHandle GetCurrentFeatureImpl(void);
  virtual gstRecordHandle GetCurrentAttributeImpl(void);
  virtual void IncrementReadingImpl(void);
  virtual bool IsReadingDoneImpl(void) {
    return DefaultIsReadingDoneImpl();
  }

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t fidx);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t fidx);
  FORWARD_GETFEATUREBOX_TO_BASE;


 protected:
  gstGeodeHandle TranslateGeometry(OGRFeature* feature) const;
  gstRecordHandle TranslateAttribute(OGRFeature* feature, std::uint32_t);

  void AddPoint(OGRPoint *point, gstPrimType type, gstGeodeHandle *baseh) const;
  int AddMultiPoint(OGRGeometry* geom,
                    gstPrimType prim_type,
                    gstGeodeHandle *baseh) const;

  void AddString(OGRLineString *string, gstPrimType type,
                 gstGeodeHandle *baseh) const;
  int AddMultiString(OGRGeometry* geom, gstPrimType type,
                     gstGeodeHandle *baseh) const;

  gstGeodeHandle GetPolygon(OGRPolygon *poly,
                            gstPrimType type) const;
  gstGeodeHandle GetMultiPolygon(OGRMultiPolygon *multi_poly,
                                 gstPrimType multi_type) const;

  void AddExteriorRing(OGRLinearRing* ring, gstPrimType type,
                       gstGeodeHandle *baseh) const;
  void AddInteriorRing(OGRLinearRing* ring, gstPrimType type,
                       gstGeodeHandle *baseh) const;

  void AddRing(OGRLinearRing* ring, gstGeode *base) const;
  void AddInvertedRing(OGRLinearRing* ring, gstGeode *base) const;

  gstStatus AddOGRLayer(int layer, OGRLayer* poLayer);
  OGRLayer* GetOGRLayer(int layer) const;

  GDALDataset* data_source_;

  std::vector<int> feature_id_offset_;

  // sequential access
  std::uint32_t currentLayer;
  OGRFeature* currentFeature;
  std::vector<OGRLayer*> layers;
};

class gstOGRSQLFormat : public gstOGRFormat {
 public:
  explicit gstOGRSQLFormat(const char *fname);
  virtual ~gstOGRSQLFormat(void);

  virtual gstStatus OpenFile();

  virtual const char* FormatName() { return "OGR SQL Query"; }

 private:
  // override to always throw
  // this format cannot support the random access API
  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t, std::uint32_t);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t, std::uint32_t);

  OGRLayer *sqlOGRLayer;
};



#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTOGRFORMAT_H__
