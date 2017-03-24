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

#ifndef KHSRC_FUSION_GST_GSTSPATIAL_H__
#define KHSRC_FUSION_GST_GSTSPATIAL_H__

#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>

class gstSpatialRef {
 public:
  gstSpatialRef(OGRSpatialReference* incoming);
  ~gstSpatialRef();

  bool Status() { return status_; }

  bool Transform(double* x, double* y, double* z) const;

  OGRSpatialReference* Incoming() const { return incoming_; }

 private:
  bool status_;

  OGRSpatialReference lat_long_;
  OGRSpatialReference* incoming_;
  OGRCoordinateTransformation* coord_trans_;
};

#endif  // !KHSRC_FUSION_GST_GSTSPATIAL_H__
