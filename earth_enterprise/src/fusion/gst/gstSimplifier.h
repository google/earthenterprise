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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLIFIER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLIFIER_H_

#include <gstGeode.h>

// Line simplification.  This implements Douglas-Peucker line
// simplification over geodes (gstPolyLine, gstRoad, and gstPolygon).
class gstSimplifier {
 public:
  gstSimplifier() : threshold_(), allowable_error_() { }

  // Set the standard number of pixels of error allowable at a given
  // level of detail.  In practice this may be overridden by packet size
  // requirements and triangle budget.
  void set_pixel_error(double error) { allowable_error_ = error; }

  // Set up the simplifier to work at the given level of detail.
  // This initializes the threshold so that simplification should
  // not produce changes in a geode which are detectable when that
  // level of detail is displayed.  Loopcount allows more aggressive
  // simplification if undetectable changes are not sufficient to
  // reduce the packet size enough.
  void ComputeThreshold(int level, int loopcount);

  // Simplify the given geode.  This does not change the geode,
  // only returns a list of the vertices that would remain in log.
  // This list is in descending order of importance (with respect
  // to satisfying the error bound).  Thus, any n-prefix of log is
  // the "best" approximation to n vertices (best with respect to
  // Douglas-Peucker, but not optimal in any geometric sense).
  // Return the max error of the simplification result.
  double Simplify(gstGeodeHandle geodeh, std::vector<int>* log) const;

  // Whether the given geode is smaller than a pixel at the current
  // resolution and level of detail.  Used for culling geodes at
  // coarse levels of detail.
  bool IsSubpixelFeature(const gstGeodeHandle &geodeh) const;

 private:
  // Current simplification threshold (product coordinates)
  double threshold_;

  // Current "weak" simplification threshold (product coordinates) to cull the
  // vertices that are insignificant for display.
  double threshold_weak_;

  // Allowable pixel-level error at a given level of detail.
  double allowable_error_;
};


// Class gstFeatureCuller checks if feature diameter is less than what is
// specified by pixel-error at a given level of detail.
// Used to cull geode at coarse levels of detail if it is too small to affect
// the display.
class gstFeatureCuller {
 public:
  gstFeatureCuller() : threshold_(), allowable_error_() { }

  // Set the standard number of pixels of error allowable at a given
  // level of detail.
  void set_pixel_error(double error) { allowable_error_ = error; }

  // Set up the FeatureCuller to work at the given level of detail.
  void ComputeThreshold(int level);

  // Whether the given geode is smaller than a specified pixel-error at
  // the current resolution and level of detail.
  bool IsSubpixelFeature(const gstGeodeHandle &geodeh) const;

 private:
  // Current culling threshold at a given level of detail.
  double threshold_;
  // Specified allowable pixel-error at a given level of detail.
  double allowable_error_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLIFIER_H_
