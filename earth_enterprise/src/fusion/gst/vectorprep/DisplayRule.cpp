// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "fusion/gst/vectorprep/DisplayRule.h"

#include <vector>
#include <map>
#include <set>
#include <list>

#include "common/khConstants.h"
#include "common/quadtreepath.h"
#include "common/khException.h"
#include "fusion/gst/gstConstants.h"
#include "fusion/gst/gstSelector.h"
#include "fusion/gst/gstSourceManager.h"
#include "fusion/gst/gstBoxCutter.h"

#include "fusion/gst/vectorprep/Binders.h"
#include "fusion/gst/vectorprep/PolylineJoiner.h"


namespace vectorprep {


// ****************************************************************************
// ***  DisplayRuleBase
// ****************************************************************************
DisplayRuleBase::~DisplayRuleBase(void) {
}

void DisplayRuleBase::CreateBinders(std::vector<FieldBinderDef> &defs,
                               khDeletingVector<Binder> *binders,
                               const QString &context) {
  binders->reserve(defs.size());
  for (unsigned int i = 0; i < defs.size(); ++i) {
    defs[i].errorContext = context + defs[i].errorContext;
    switch (defs[i].fieldGen.mode) {
      case FieldGenerator::RecordFormatter:
        binders->push_back(new RecordFormatterBinder(defs[i], srcHeader));
        break;
      case FieldGenerator::JSExpression:
        binders->push_back(new JSBinder(defs[i], jsCX));
        break;
    }
  }
}

gstHeaderHandle DisplayRuleBase::CreateBoundHeader(
    const std::vector<FieldBinderDef> &defs) {
  gstHeaderHandle header = gstHeaderImpl::Create();
  for (unsigned int i = 0; i < defs.size(); ++i) {
    header->addSpec(defs[i].fieldName, defs[i].fieldType);
  }
  return header;
}

bool DisplayRuleBase::Prepare(const gstBBox &cutBox,
                              unsigned int level,
                              FeatureTile *featureOut,
                              SiteTile *siteOut,
                              const vectorquery::DisplayRuleTile &in) {
  if (featureOut && !featureValidLevels.Contains(level)) {
    notify(NFY_VERBOSE, "Skipping features at level %d (non-valid level)",
           static_cast<int>(level));
    featureOut = 0;
  }

  if (siteOut && !siteValidLevels.Contains(level)) {
    notify(NFY_VERBOSE, "Skipping sites at level %d (non-valid level)",
           static_cast<int>(level));
    siteOut = 0;
  }

  // reate BoxCutter processor and initialize by cutBox.
  // Further use it many times;
  fusion_gst::BoxCutter box_cutter(cutBox, false);  // cut_holes is false.

  // ***** process each selected feature
  for (unsigned int i = 0; i < in.selectedIds.size(); ++i) {
    int featureId = in.selectedIds[i];

    // --- load geometry from disk
    gstGeodeHandle geode = theSourceManager->GetFeatureOrThrow(
        UniqueFeatureId(source->Id(), 0 /* layerId */, featureId), false);

    // --- process Site geometry
    // do this first in case the geometry needs to get converted for
    // the feature
    bool haveSite = false;
    gstVertex v;
    if (siteOut) {
      v = GetSiteLocation(geode);
      if (cutBox.Contains(v)) {
        haveSite = true;
      }
    }

    // --- process Feature geometry
    bool haveFeature = false;
    GeodeList pieces;
    if (featureOut) {
      // covert geometry to the type we want for rendering
      ConvertFeatureType(geode);

      // Flatten the geometry.
      // if (cutBox.Intersect(geode->BoundingBox()) &&
      //    geode->Flatten(&pieces) != 0) {

      // As SKIA barfs on non-cut geometry, cutting geometry as a work-around.
      // cut the geometry against our cutbox
      // TODO: when skia fixes this, need to revert to flatten as
      // above, as with cut-geometries it will be impossible to draw the shields
      // in the boundary properly.
      if (box_cutter.Run(geode, pieces) != 0) {
        haveFeature = true;
      }
    }

    // ----- bind (expand/evaluate) all the fields necessary for both sites
    // and features
    gstRecordHandle boundFeatureRec;
    gstRecordHandle boundSiteRec;
    if ((haveSite && siteBinders.size()) ||
        (haveFeature && featureBinders.size())) {
      // fetch the source record
      gstRecordHandle srcRec = source->GetAttributeOrThrow(
          0 /* layerId */, featureId);

      // bind for the site
      if (haveSite && siteBinders.size()) {
        assert(siteOut);
        boundSiteRec = siteBoundHeader->NewRecord();
        for (unsigned int i = 0; i < siteBinders.size(); ++i) {
          siteBinders[i]->Bind(boundSiteRec, srcRec, i);
        }
      }

      // bind for the feature
      if (haveFeature && featureBinders.size()) {
        assert(featureOut);
        boundFeatureRec = featureBoundHeader->NewRecord();
        for (unsigned int i = 0; i < featureBinders.size(); ++i) {
          featureBinders[i]->Bind(boundFeatureRec, srcRec, i);
        }
      }
    }

    // NOTE: After partitioning we do not have multi-polygon-features.
    // And all features contain only one part. It is checked in cycle below
    // and there is no additional checks further.

    // --- Add feature pieces and boundRec to the list for this tile
    if (haveFeature) {
      assert(featureOut);
      for (GeodeListIterator piece = pieces.begin();
           piece != pieces.end(); ++piece) {
        if ((*piece)->PrimType() == gstMultiPolygon ||
            (*piece)->PrimType() == gstMultiPolygon25D ||
            (*piece)->PrimType() == gstMultiPolygon3D) {
          notify(NFY_FATAL,
                 "%s: Incorrect geometry after partitioning.", __func__);
        }

        featureOut->glist.push_back(*piece);
        featureOut->rlist.push_back(boundFeatureRec);
      }
    }

    // --- Add site vertex and boundRec to the list for this tile
    if (haveSite) {
      assert(siteOut);
      siteOut->vlist.push_back(v);
      siteOut->rlist.push_back(boundSiteRec);
    }
  }  // for each feature

  // reduce the feature geometry (if we have any to reduce)
  if (featureOut && featureOut->glist.size()) {
    ReduceFeatures(level, featureOut);
  }

  // reduce the site geometry (if we have any to reduce)
  if (siteOut && siteOut->vlist.size()) {
    ReduceSites(siteOut);
  }

  return ((featureOut && featureOut->glist.size() > 0) ||
          (siteOut && siteOut->vlist.size() > 0));
}


gstVertex DisplayRuleBase::GetSiteLocation(const gstGeodeHandle &feature) {
  double offset_x_ = 0;
  double offset_y_ = 0;

  if (feature->IsDegenerate()) {
    notify(NFY_WARN,
           "Can't build a site without a feature to get coords from!");
    return gstVertex();
  }

  //
  // find the vertex coordinate
  //
  if (feature->PrimType() == gstPoint) {
    return feature->Center();
  } else {
    switch (siteDisplayPosition) {
      case VectorDefs::AreaCenter:
        {
          gstVertex center = feature->Center();
          if (center != gstVertex()) {
            return center;
          } else {
            return gstVertex(feature->BoundingBox().CenterX() + offset_x_,
                             feature->BoundingBox().CenterY() + offset_y_);
          }
        }
        break;

      case VectorDefs::LineCenter:
        {
          // TODO: Feature can contain more than one polyline.
          // Where should be the line center in this case?
          return feature->Center();
        }
        break;
    }
  }

  return gstVertex();
}

void DisplayRuleBase::ConvertFeatureType(gstGeodeHandle &geode) {
  if (featureDisplayType == VectorDefs::PointZ ||
      featureDisplayType == VectorDefs::IconZ) {
    // if the feature type is point then just return true
    // coz preparesites has already done the conversion

    // TODO: - validate statement above for things other than streamed layers
    return;
  } else if (featureDisplayType == VectorDefs::LineZ) {
    if (geode->PrimType() == gstPolyLine) {
      if (featureReduceMethod == VectorDefs::ReduceFeatureRoads) {
        geode->ChangePrimType(gstStreet);
      }
      return;
    } else if (geode->PrimType() == gstPolyLine25D) {
      if (featureReduceMethod == VectorDefs::ReduceFeatureRoads) {
        geode->ChangePrimType(gstStreet25D);
      }
      return;
    } else if (geode->FlatPrimType() == gstStreet) {
      // gstStreet is Fusion internal type, we can't get it from source
      // data set.
      assert(featureReduceMethod == VectorDefs::ReduceFeatureRoads);
      return;
    } else if (geode->PrimType() == gstPolygon) {
      geode->ChangePrimType(
          featureReduceMethod == VectorDefs::ReduceFeatureRoads ?
          gstStreet : gstPolyLine);
      return;
    } else if (geode->PrimType() == gstPolygon25D ||
               geode->PrimType() == gstPolygon3D) {
      geode->ChangePrimType(
          featureReduceMethod == VectorDefs::ReduceFeatureRoads ?
          gstStreet25D : gstPolyLine25D);
      return;
    } else if (geode->PrimType() == gstMultiPolygon) {
      gstGeodeHandle new_geode;
      geode->ChangePrimType(
          featureReduceMethod == VectorDefs::ReduceFeatureRoads ?
          gstStreet : gstPolyLine, &new_geode);
      geode.swap(new_geode);
      return;
    } else if (geode->PrimType() == gstMultiPolygon25D ||
               geode->PrimType() == gstMultiPolygon3D) {
      gstGeodeHandle new_geode;
      geode->ChangePrimType(
          featureReduceMethod == VectorDefs::ReduceFeatureRoads ?
          gstStreet25D : gstPolyLine25D, &new_geode);
      geode.swap(new_geode);
      return;
    }

  } else if (featureDisplayType == VectorDefs::PolygonZ) {
    if (geode->PrimType() == gstPolyLine) {
      geode->ChangePrimType(gstPolygon);
      return;
    } else if (geode->PrimType() == gstPolyLine25D) {
      geode->ChangePrimType(gstPolygon25D);
      return;
    } else if (geode->PrimType() == gstPolygon ||
               geode->PrimType() == gstPolygon25D ||
               geode->PrimType() == gstPolygon3D ||
               geode->PrimType() == gstMultiPolygon ||
               geode->PrimType() == gstMultiPolygon25D ||
               geode->PrimType() == gstMultiPolygon3D) {
      return;
    }
  }

  throw khException(kh::tr("Invalid conversion from asset type (%1)"
                           " to diplay type (%2).")
                    .arg(PrettyPrimType(geode->PrimType()).c_str())
                    .arg(VectorDefs::PrettyFeatureDisplayType
                         (featureDisplayType).ascii()));
}

void DisplayRuleBase::ReduceFeatures(unsigned int level, FeatureTile *tile) {
  switch (featureReduceMethod) {
    case VectorDefs::ReduceFeatureNone:
      // do nothing
      break;
    case VectorDefs::ReduceFeaturePolylines:
      ReducePolylines(tile);
      break;
    case VectorDefs::ReduceFeatureRoads:
      {
        bool remove_overlapping_segments =
            level < kAvoidRemovingOverlappingRoadSegmentsAtOrHigherZoomLevel;
        // levels lower than 13 get overlapping segments removed
        // this is based on a tolerance derived from the level
        ReduceRoads(remove_overlapping_segments, level, tile);
      }
      break;
    case VectorDefs::ReduceFeaturePolygons:
      ReducePolygons(tile);
      break;
  }
}

void DisplayRuleBase::ReduceSites(SiteTile *tile) {
  switch (siteReduceMethod) {
    case VectorDefs::ReduceSiteNone:
      // do nothing
      break;
    case VectorDefs::ReduceSiteSuppressDuplicates:
      gstSelector::RemoveDuplicateSites(&tile->vlist, &tile->rlist);
      break;
  }
}

void DisplayRuleBase::ReducePolylines(FeatureTile *tile) {
  // reducing lines means reduce roads, but exclude overlapping segments filter
  // Pass level 0 to mean invalid level
  const bool remove_overlapping_segments = false;
  ReduceRoads(remove_overlapping_segments, 0, tile);
}

void DisplayRuleBase::ReduceRoads(bool remove_overlapping_segments,
                                  std::uint32_t level, FeatureTile *tile) {
  //
  // sort all features by name
  //
  std::map<QString, GeodeList > name_map;
  GeodeListIterator geode = tile->glist.begin();
  RecordListIterator boundRec = tile->rlist.begin();
  const QString noName;
  for (; geode != tile->glist.end(); ++geode, ++boundRec) {
    // Due to choice of display rule at a particular zoom level a road may not
    // have any labels attached to it. Take care of that, rather than crashing!
    // Note: All noName segments will be considered as one set for
    // RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices
    if (*boundRec) {
      assert(featureKeyFieldNum >= 0 &&
             featureKeyFieldNum < static_cast<std::int32_t>((*boundRec)->NumFields()));
      name_map[(*boundRec)->Field(featureKeyFieldNum)->ValueAsUnicode()]
          .push_back(*geode);
    } else {
      name_map[noName].push_back(*geode);
    }
  }

  //
  // join within each name set
  //
  for (std::map<QString, GeodeList>::iterator nameset =
           name_map.begin(); nameset != name_map.end(); ++nameset) {
    RemoveDuplicateSegmentsAndJoinSegments(nameset->second);
  }


  if (remove_overlapping_segments) {
    for (std::map<QString, GeodeList>::iterator nameset =
             name_map.begin(); nameset != name_map.end(); ++nameset) {
      GeodeList glist;
      for (GeodeListIterator g = nameset->second.begin();
           g != nameset->second.end(); ++g) {
        if (!(*g)->IsEmpty())
          glist.push_back(*g);
      }
      RemoveOverlappingSegments(&glist, level);
    }
  }

  gstSelector::RemoveEmptyFeatures(&tile->glist, &tile->rlist,
                                   kMinPolylineVertices);
}

void DisplayRuleBase::ReducePolygons(FeatureTile *tile) {
  gstSelector::RemoveEmptyFeatures(&tile->glist, &tile->rlist,
                                   kMinCycleVertices);
}


// Note: each segment is a polyline with two end points and we treat it as a
// segment with two end points for the purpose of JoinSegments.
// First we remove duplicates, then we join at vertices having degree two.
template<class T>
void DisplayRuleBase::RemoveDuplicateSegmentsAndJoinSegments(
    const T& glist) {
  std::uint64_t num_duplicates = 0;
  std::uint64_t num_joined = 0;
  PolylineJoiner<T>::RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
      glist, &num_duplicates, &num_joined);
}

//
// extensive analysis by hand has resulted in
// choosing a constant of 28 to add to the level
// where we can remove overlapping segments
//
#define OVERLAP_CONSTANT 28

void DisplayRuleBase::RemoveOverlappingSegments(GeodeList* glist, int level) {
  double epsilon = 1.0 / (1LLU << (level + OVERLAP_CONSTANT));

  int removed_count = 0;
  for (GeodeList::iterator g1 = glist->begin();
       g1 != glist->end(); ++g1) {
    if ((*g1)->IsEmpty())
      continue;

    gstGeode *geode1 = static_cast<gstGeode*>(&(**g1));

    GeodeList::iterator g2 = g1;
    for (++g2; g2 != glist->end(); ++g2) {
      if ((*g2)->IsEmpty())
        continue;

      gstGeode *geode2 = static_cast<gstGeode*>(&(**g2));

      if (geode1->Overlaps(geode2, epsilon)) {
        geode2->Clear();
        ++removed_count;
      } else if (geode2->Overlaps(geode1, epsilon)) {
        geode1->Clear();
        ++removed_count;
        break;
      }
    }
  }
}

}  // namespace vectorprep
