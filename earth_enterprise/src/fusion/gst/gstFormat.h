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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTFORMAT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTFORMAT_H_

#include <limits.h>

#include <string>

#include "fusion/gst/gstTypes.h"
#include "common/khArray.h"
#include "common/khTileAddr.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstMemory.h"

class gstRegistry;
class QTextCodec;

// This assumes the coordinates are
// in Lat/Lon projection, with WGS84 datum
#define ND_I 1 / 360.
#define CONV_DEG(deg) ((deg) + 180) * ND_I


extern char *getStr(const char *buf, int nbuf, char *obuf);
extern void strLeftShift(char *buf);
extern int getInt(const char *buf, int nbuf);
extern std::uint64_t getUInt64(const char *buf, int nbuf);
extern double getDouble(const char *buf, int nbuf);

#define GET_INT(_field) getInt(_field, sizeof _field)
#define GET_UINT64(_field) getUInt64(_field, sizeof _field)
#define GET_DOUBLE(_field) getDouble(_field, sizeof _field)
#define GET_STR(_field, _buf) getStr(_field, sizeof _field, _buf)

extern int getIntB(const char *buf, int nbuf);
extern int getIntB_le(const unsigned char *buf, int nbuf);
#define GET_INT_B(_field) getIntB(_field, sizeof _field)
#define GET_INT_B_LE(_field) getIntB_le(_field, sizeof _field)

struct gstFormatToken {
  int tag;
  char *desc;
};

class gstFormat;
class gstSource;
class gstSpatialRef;

typedef void (*gstFormatInitFunc)();
typedef gstFormat *(*gstFormatNewFmtFunc)(const char *);

#define FMT_LINEBUF 1024

class gstLayerDef {
 public:
  gstLayerDef(gstPrimType ftype, std::uint32_t fcount, const gstHeaderHandle& header);
  gstLayerDef(std::uint32_t fcount, const gstHeaderHandle& header);
  ~gstLayerDef();

  void SetType(gstPrimType t) { type_ = t; }
  gstPrimType Type() const { return type_; }

  void SetNumFeatures(std::uint32_t f) { num_features_ = f; }
  std::uint32_t NumFeatures() const { return num_features_; }

  void SetAverageFeatureDiameter(double d) {
    average_feature_diameter_ = d;
  }

  double AverageFeatureDiameter() const {
    return average_feature_diameter_;
  }

  void SetMinResolutionLevel(std::uint32_t min_resolution_level) {
    min_resolution_level_ = min_resolution_level;
  }

  std::uint32_t MinResolutionLevel() const {
    return min_resolution_level_;
  }

  void SetMaxResolutionLevel(std::uint32_t max_resolution_level) {
    max_resolution_level_ = max_resolution_level;
  }

  std::uint32_t MaxResolutionLevel() const {
    return max_resolution_level_;
  }

  void SetEfficientResolutionLevel(std::uint32_t efficient_resolution_level) {
    efficient_resolution_level_ = efficient_resolution_level;
  }

  std::uint32_t EfficientResolutionLevel() const {
    return efficient_resolution_level_;
  }

  void SetAttrDefs(const gstHeaderHandle& h) { attrib_defs_ = h; }
  const gstHeaderHandle& AttrDefs() const { return attrib_defs_; }

  void SetBoundingBox(const gstBBox& box);
  const gstBBox& BoundingBox() const { return bounding_box_;}

  void SetCodec(QTextCodec* codec) { attrib_defs_->SetCodec(codec); }

 private:
  gstPrimType type_;
  std::uint32_t num_features_;
  gstHeaderHandle attrib_defs_;
  gstBBox bounding_box_;
  // The average feature diameter at level 0. It is calculated based on
  // average feature diameters of sources (product coordinates).
  // The parameterization of the earth is within the unit square [0, 1]x[0, 1].
  double average_feature_diameter_;

  // The (min, max, efficient) resolution levels calculated by (max, min,
  // average) feature size.
  std::uint32_t  min_resolution_level_;
  std::uint32_t  max_resolution_level_;
  std::uint32_t  efficient_resolution_level_;
};

class gstFormat : public gstMemory {
 public:
  explicit gstFormat(const char *n);
  gstFormat(const char *n, bool is_mercator_imagery);
  virtual ~gstFormat();

  void SetCodec(QTextCodec* codec);

  // Each gstFormat-object is used only once to OpenFile. It is ensured in
  // gstSource::Open() and also by private access to gstFormat::Open().
  virtual gstStatus OpenFile() = 0;

  // TODO: remove CloseFile()-function from gstFormat. Currently
  // the virtuality of this function is not used. Mostly it is called in dtor
  // of class inherited from gstFormat.
  virtual gstStatus CloseFile() = 0;

 private:
  friend class gstSource;
  static gstFormat* Open(const std::string &path);

  // ===== new sequential access api =====
  // Classes that don't directly support seqential access can use the
  // FORWARD_ALL_SEQENTIAL_ACCESS_TO_BASE macro to generate all the
  // forwarding functions for these pure virtuals
  virtual void ResetReadingImpl(std::uint32_t layer) = 0;
  gstGeodeHandle GetNormCurrFeatureImpl(void);
  virtual gstGeodeHandle GetCurrentFeatureImpl(void) = 0;
  virtual gstRecordHandle GetCurrentAttributeImpl(void) = 0;
  virtual void IncrementReadingImpl(void) = 0;
  virtual bool IsReadingDoneImpl(void) = 0;

  // ===== older random access API =====
  // Classes that don't directly support fetching of BBoxes separate from
  // features can use the FORWARD_GETFEATUREBOX_TO_BASE macro to generate
  // the forwarding function
  gstGeodeHandle GetNormFeatureImpl(std::uint32_t layer, std::uint32_t id,
                                    bool is_mercator_preview);
  virtual gstGeodeHandle  GetFeatureImpl(std::uint32_t layer, std::uint32_t id) = 0;
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id) = 0;
  virtual gstBBox         GetFeatureBoxImpl(std::uint32_t layer, std::uint32_t id) = 0;

 protected:
  // default implementation for sequential access API
  void DefaultResetReadingImpl(std::uint32_t layer);
  gstGeodeHandle DefaultGetCurrentFeatureImpl(void);
  gstRecordHandle DefaultGetCurrentAttributeImpl(void);
  void DefaultIncrementReadingImpl(void);
  bool DefaultIsReadingDoneImpl(void);

  // default implementation for random access API
  gstBBox DefaultGetFeatureBoxImpl(std::uint32_t layer, std::uint32_t id);

  void TransformGeode(gstGeodeHandle g) const;
  void TransformGeode(gstGeode *g) const;

 public:
  virtual void setEnabled(bool e) { ; }

  virtual const char* FormatName() { return "Base Format"; }

  // convenience routines
  char* getNextLine(FILE *fp);

  // format description
  virtual gstFormatDesc* description(int& c) {
    c = 0;
    return NULL;
  }

  static void itemInt(int f, va_list ap)
  { *va_arg(ap, int*) = f; }
  static void itemFloat(float f, va_list ap)
  { *va_arg(ap, float*) = f; }
  static void itemDouble(double f, va_list ap)
  { *va_arg(ap, double*) = f; }
  static void itemString(char *f, va_list ap)
  { *va_arg(ap, char**) = f; }

  int suffixCheck(const char *nm, const char *sfx) {
    return (strlen(nm) > strlen(sfx)
            && !strcmp(nm+(strlen(nm) - strlen(sfx)), sfx));
  }

  std::uint32_t NumLayers() const {
    return layer_defs_.size();
  }

  std::uint32_t NumFeatures(std::uint32_t layer) const {
    return layer_defs_[layer].NumFeatures();
  }

  gstSpatialRef* SpatialRef() const { return spatial_ref_; }
  void SetSpatialRef(gstSpatialRef* srs);

  std::vector<gstLayerDef> GetNormLayerDefs() const;

  bool IsMercatorImagery() const { return is_mercator_imagery_; }

 protected:
  gstLayerDef& AddLayer(gstPrimType t, std::uint32_t n, const gstHeaderHandle& h);
  gstLayerDef& AddLayer(std::uint32_t n, const gstHeaderHandle& h);
  void AddLayer(const gstLayerDef& l);

 private:
  void VertexTransform(double* x, double* y) const;
  gstVertex TransformVertex(const gstVertex& v) const;

 protected:
  static int ParseToken(char*, char**, gstFormatToken *);

  // If is_mercator_imagery_ is true latitude_longitude is in Meter else it is
  // in degree
  double NormalizeLatitude(double latitude) const {
    return (is_mercator_imagery_
      ? MercatorProjection::FromMercatorNormalizedLatitudeToNormalizedLatitude(
                            khTilespace::NormalizeMeter(latitude))
      : khTilespace::Normalize(latitude));
  }
  double NormalizeLongitude(double longitude) const {
    return (is_mercator_imagery_
      ? khTilespace::NormalizeMeter(longitude)
      : khTilespace::Normalize(longitude));
  }

  char _lineBuf[FMT_LINEBUF];

  gstSpatialRef *spatial_ref_;
  std::vector<gstLayerDef> layer_defs_;

  bool nofileok;

  ///////////////////////////////////////////////////////////////
  std::uint32_t seqReadLayer;
  std::uint32_t seqReadCurrID;
  bool seqReadIsDone;

  bool noXForm;
  bool is_mercator_imagery_;
};


#define FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE           \
virtual void ResetReadingImpl(std::uint32_t layer) {           \
  DefaultResetReadingImpl(layer);                       \
}                                                       \
virtual gstGeodeHandle GetCurrentFeatureImpl(void) {    \
  return DefaultGetCurrentFeatureImpl();                \
}                                                       \
virtual gstRecordHandle GetCurrentAttributeImpl(void) { \
  return DefaultGetCurrentAttributeImpl();              \
}                                                       \
virtual void IncrementReadingImpl(void) {               \
  DefaultIncrementReadingImpl();                        \
}                                                       \
virtual bool IsReadingDoneImpl(void) {                  \
  return DefaultIsReadingDoneImpl();                    \
}


#define FORWARD_GETFEATUREBOX_TO_BASE                           \
virtual gstBBox GetFeatureBoxImpl(std::uint32_t layer, std::uint32_t id) {    \
  return DefaultGetFeatureBoxImpl(layer, id);                   \
}

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTFORMAT_H_
