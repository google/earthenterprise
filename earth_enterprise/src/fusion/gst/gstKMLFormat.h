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

// Fixed polygon ingestion: added logic to determine type of ingested
// polygon (2D/2.5D) and primary type for layer based on dominant geometry type.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTKMLFORMAT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTKMLFORMAT_H_

#include <string>
#include <vector>

#include <khxml/khdom.h>

#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstFormat.h"

// Kml geometry types.
enum gstKmlGeomType {
  kKmlUnknown = 0,
  kKmlPoint = 1,
  kKmlLineString = 2,
  kKmlPolygon = 3
};

class gstKMLFormat : public gstFormat {
 public:
  explicit gstKMLFormat(const char *n);
  ~gstKMLFormat();

  virtual gstStatus OpenFile();
  virtual gstStatus CloseFile();

  virtual const char* FormatName() { return "Keyhole Markup Language"; }

 private:
  FORWARD_ALL_SEQUENTIAL_ACCESS_TO_BASE;

  virtual gstGeodeHandle GetFeatureImpl(std::uint32_t layer, std::uint32_t id);
  virtual gstRecordHandle GetAttributeImpl(std::uint32_t layer, std::uint32_t id);

  FORWARD_GETFEATUREBOX_TO_BASE;

  void GetElementsByTagName(khxml::DOMNode* parent, const std::string& tagname,
                            khDOMElemList& elements) const;

  void ExtractFeature(const gstKmlGeomType type, khxml::DOMElement* point,
                      gstGeodeHandle& geode);

  void AddPoint(khxml::DOMElement* elem, gstGeodeHandle& geodeh);

  void AddLineString(khxml::DOMElement* elem, gstGeodeHandle& geodeh);

  void AddPolygon(khxml::DOMElement* elem, gstGeodeHandle& geodeh);

  void AddOuterBoundary(const gstVertexVector& verts, gstPrimType type,
                        gstGeodeHandle& geodeh) const;

  void AddInnerBoundary(const gstVertexVector& verts, gstPrimType type,
                        gstGeodeHandle& geodeh) const;

  // Whether the feature has 2.5D coordinates extension.
  bool ExtractCoordinates(khxml::DOMElement* elem,
                          gstVertexVector& verts) const;

  void ParseKml(khxml::DOMNode* root);

  gstStatus ParseKml(const std::string& kml_file);

  int ExtractKml(const std::string& kmz_file);
  //  QString GetAllChildren(khxml::DOMNode* node);

 private:
  // Computes primary type based on dominant feature type.
  gstPrimType GetPrimType() const;

  // Path to temporary directory.
  static const std::string TMPDIR;
  // Name of kml-file located in kmz.
  static const std::string ZKMLFILE;

  // Path to temporary kml-file extracted from .kmz.
  std::string tmp_kml_file_;
  bool is_kmz_;

  // Auxiliary data to determine layer primary type.
  gstKmlGeomType type_;  // Dominant feature type.

  gstHeaderHandle header_;
  GeodeList geodes_;
  std::vector<gstRecordHandle> records_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTKMLFORMAT_H_
