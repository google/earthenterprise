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


#include "fusion/gst/gstSelector.h"

#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <set>

#include <qstringlist.h>

#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "common/khTimer.h"
#include "common/khProgressMeter.h"
#include "fusion/autoingest/.idl/storage/VectorFuseAssetConfig.h"

#include "fusion/gst/gstConstants.h"
#include "fusion/gst/gstFilter.h"
#include "fusion/gst/gstSource.h"
#include "fusion/gst/gstSourceManager.h"
#include "fusion/gst/gstProgress.h"
#include "fusion/gst/gstFeature.h"
#include "fusion/gst/gstSite.h"
#include "fusion/gst/gstFileUtils.h"
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstKVPTable.h"
#include "fusion/gst/gstKVPFile.h"
#include "fusion/gst/gstQuadAddress.h"
#include "fusion/gst/gstJobStats.h"
#include "fusion/gst/vectorprep/PolylineJoiner.h"
#include "fusion/gst/gstBoxCutter.h"
#include "fusion/gst/gstSequenceAlg.h"
#include "fusion/gst/gstLayer.h"

#define COMPLETELY_COVERED_OPT 1

// Used when adding a layer to the project in the GUI
gstSelector::gstSelector(gstSource *src, int srcLayerNum,
                         const LayerConfig& lcfg,
                         const QStringList &externalContextScripts_)
    : gstMemory(NULL),
      source_id_(theSourceManager->AddSource(src)),
      layer_(srcLayerNum),
      config(lcfg, externalContextScripts_),
      gst_layer_(NULL),   // it is not used.
      query_complete_(false) {
  for (std::vector<DisplayRuleConfig>::const_iterator it =
           lcfg.displayRules.begin(); it != lcfg.displayRules.end(); ++it) {
    (void)CreateFilter(*it);
  }
}

// used from gevectorquery
gstSelector::gstSelector(gstSource *src, const QueryConfig& cfg)
    : gstMemory(NULL),
      source_id_(theSourceManager->AddSource(src)),
      layer_(0),
      config(cfg),
      gst_layer_(NULL),   // it is not used.
      query_complete_(false) {
  for (std::vector<FilterConfig>::const_iterator it = cfg.filters.begin();
       it != cfg.filters.end(); ++it) {
    (void)CreateFilter(*it);
  }
}

// used from gevectorfuse
gstSelector::gstSelector(
    gstLayer* gst_layer, gstSource *src, const FuseConfig& cfg)
    : gstMemory(NULL),
      source_id_(theSourceManager->AddSource(src)),
      layer_(0),
      config(),  // our config is not used when doing a fuse
      // Used for callback to collect covered quads statistics.
      gst_layer_(gst_layer),
      query_complete_(false) {
  for (std::vector<FuseItem>::const_iterator it = cfg.items.begin();
       it != cfg.items.end(); ++it) {
    gstFilter* filter = CreateFilter(*it);
    filter->ReadQueryResultsFile(it->filename.c_str(), source_id_);
  }
  query_complete_ = true;
}

gstSelector::~gstSelector() {
  RemoveAllFilters();
  theSourceManager->RemoveSource(source_id_);
}

// Called from QUI when LayerConfig is updated for any reason
void gstSelector::SetConfig(const LayerConfig &newlcfg, bool &keptQueries,
                            const QStringList &externalContextScripts_) {
  QueryConfig newqcfg = QueryConfig(newlcfg, externalContextScripts_);

  // determine if we can preserve the query results we already have
  bool keepQueries = queryComplete() && (newqcfg == config);

  // update my copy of the config
  config = newqcfg;

  // if necessary, grab ahold of all geoindexes from the filters
  std::vector<gstGeoIndexHandle> keepGeoIndex;
  if (keepQueries) {
    for (unsigned int f = 0; f < NumFilters(); ++f) {
      gstGeoIndexHandle index = GetFilter(f)->GetGeoIndex();
      if (!index) {
        notify(NFY_WARN, "Get %d: null geo index", f);
      }
      keepGeoIndex.push_back(GetFilter(f)->GetGeoIndex());
    }
  }

  // Blow away all the existing filters, this is easier than defining and
  // calling SetConfig all the way down the heirarchy
  RemoveAllFilters();

  // rebuild the filters w/ new config
  int f = 0;
  for (std::vector<DisplayRuleConfig>::const_iterator to =
         newlcfg.displayRules.begin();
       to != newlcfg.displayRules.end(); ++to, ++f) {
    gstFilter *filter = CreateFilter(*to);
    if (keepQueries) {
      // restore the geoIndex
      gstGeoIndexHandle index = keepGeoIndex[f];
      if (!index) {
        notify(NFY_WARN, "Set %d: null geo index", f);
      }
      filter->SetGeoIndex(keepGeoIndex[f]);
    }
  }

  // update my query_complete_ state and tell the caller if we kept the queries
  query_complete_ = keepQueries;
  keptQueries = keepQueries;
}

gstSource* gstSelector::getSource() const {
  return theSourceManager->GetSource(source_id_);
}

gstRecordHandle gstSelector::getPickRecord(unsigned int id) {
  // we don't have to worry about GetFeature failing since we are pulling
  // from our pick_list and it can only get into the pick_list if it has a
  // valid feature
  // But we do need to contain exceptions just in case
  try {
    return theSourceManager->GetAttributeOrThrow(
        UniqueFeatureId(source_id_, layer_, pick_list_[id]));
  } catch(...) {
    assert(0);
  }
  return gstRecordHandle();
}

gstGeodeHandle gstSelector::getPickGeode(unsigned int id) {
  // we don't have to worry about GetFeature failing since we are pulling
  // from our pick_list and it can only get into the pick_list if it has a
  // valid feature
  // But we do need to contain exceptions just in case
  try {
    return theSourceManager->GetFeatureOrThrow(
        UniqueFeatureId(source_id_, layer_, pick_list_[id]), false);
  } catch(...) {
    assert(0);
  }
  return gstGeodeHandle();
}

void gstSelector::applyBox(const gstDrawState& state, int mode) {
  if (mode == PICK_CLEAR_ADD)
    pick_list_.clear();

  //
  // only pick from active features by
  // stepping through each filter and look for matches
  //
  for (unsigned int f = 0; f < NumFilters(); ++f) {
    // skip if not visible
    if (!GetFilter(f)->IsVisible(state))
      continue;

    SelectList subset;
    if (state.IsMercatorPreview()) {
      gstBBox box = state.select;
      MercatorProjection::NormalizeToFlatFromMercator(&box.n, &box.s);
      GetFilter(f)->IntersectBBox(box, &subset);
    } else {
      GetFilter(f)->IntersectBBox(state.select, &subset);
    }

    for (SelectListIterator it = subset.begin(); it != subset.end(); ++it) {
      // subset now contains a list of all the features whose bounding box
      // intersects our selection box
      // Now we need to load the actual feature and see if the feature itself
      // intersects our selection box

      // We can ignore missing/broken features here.
      // The filter will only return features that were valid when it loaded
      // them. Even if they are now broken we'd have no way to report them
      // since we're in the middle of a drag even and we cannot pop up a
      // dialog.  The worst that could happen is that the now-broken feature
      // isn't rendered as selected.
      gstGeodeHandle geode;
      try {
        geode = theSourceManager->GetFeatureOrThrow(
            UniqueFeatureId(source_id_, layer_, *it),
            state.IsMercatorPreview());
      } catch(...) {
        // silently ignore. See comment above 'try'
        continue;
      }

      if (geode->Intersect(state.select)) {
        if (mode == PICK_CLEAR_ADD) {
          pick_list_.push_back(*it);
        } else {
          std::vector<int>::iterator found = find(pick_list_.begin(),
                                                  pick_list_.end(), *it);
          if (mode == PICK_SUBTRACT && found != pick_list_.end()) {
            pick_list_.erase(found);
          } else if (mode == PICK_ADD && found == pick_list_.end()) {
            pick_list_.push_back(*it);
          }
        }
      }
    }
  }
}

void gstSelector::applyBoxCutter(const gstDrawState& state,
                                 GeodeList& cutlist) {
  // Create BoxCutter and initialize with select bounding box once.
  // Further use it many times.
  fusion_gst::BoxCutter box_cutter(state.select, false);  // cut_holes is false.

  for (unsigned int f = 0; f < NumFilters(); ++f) {
    if (!GetFilter(f)->IsVisible(state))
      continue;

    SelectList select_list;
    GetFilter(f)->IntersectBBox(state.select, &select_list);

    for (SelectListIterator it = select_list.begin();
         it != select_list.end(); ++it) {
      // we don't have to worry about GetFeature failing since we only ask for
      // ones that made it past our filter, and even if it did fail we're
      // in the middle of a draw and cannot open an error dialog
      gstGeodeHandle geode;
      try {
        geode = theSourceManager->GetFeatureOrThrow(
            UniqueFeatureId(source_id_, layer_, *it),
            state.IsMercatorPreview());
      } catch(...) {
        // silently ignore. see comment before 'try'
        continue;
      }

      GeodeList pieces;
      if (box_cutter.Run(geode, pieces) != 0) {
        std::copy(pieces.begin(), pieces.end(), std::back_inserter(cutlist));
      }
    }
  }
}

void gstSelector::UpdateSimplifyStats(unsigned int features_removed,
                                      unsigned int vertexes_removed,
                                      double max_error) {
  build_stats.culled_features += features_removed;
  build_stats.culled_vertexes += vertexes_removed;
  if (build_stats.max_simplify_error < max_error)
    build_stats.max_simplify_error = max_error;
}

void gstSelector::DumpBuildStats() {
  notify(NFY_NOTICE, "--------------------------------------");
  notify(NFY_NOTICE, "Original feature count: %Zu",
         build_stats.total_features);
  notify(NFY_NOTICE, "Original vertex count: %Zu",
         build_stats.total_vertexes);
  if (build_stats.duplicate_features > 0)
    notify(NFY_NOTICE, "  Duplicated features removed: %Zu",
           build_stats.duplicate_features);
  if (build_stats.join_count > 0)
    notify(NFY_NOTICE, "  Removed features by joining: %Zu",
           build_stats.join_count);
  if (build_stats.overlap_count > 0)
    notify(NFY_NOTICE, "  Overlapping features removed: %Zu",
           build_stats.overlap_count);

  notify(NFY_NOTICE, "Reduced feature count: %Zu",
         build_stats.reduced_features);
  notify(NFY_NOTICE, "Reduced vertex count: %Zu",
         build_stats.reduced_vertexes);
  notify(NFY_NOTICE, "  Culled features: %Zu",
         build_stats.culled_features);
  notify(NFY_NOTICE, "  Culled vertexes: %Zu",
         build_stats.culled_vertexes);
  notify(NFY_NOTICE, "  Max. Pixel Error: %lf",
         build_stats.max_simplify_error);
  notify(NFY_NOTICE, "Final feature count: %Zu",
         build_stats.reduced_features - build_stats.culled_features);
  notify(NFY_NOTICE, "Final vertex count: %Zu",
         build_stats.reduced_vertexes - build_stats.culled_vertexes);
  notify(NFY_NOTICE, "Duplicate sites removed: %Zu",
         build_stats.duplicate_sites);
  notify(NFY_NOTICE, "--------------------------------------");
  build_stats.Reset();
}


// convert the primtype of the geode based on what the config type
// selected by the user. we currently allow only line <--> polygon,
// line -> street conversions.
bool gstSelector::convertFeatureType(
    gstGeodeHandle *geode,
    const gstFeatureConfigs& feature_configs) {
  bool ret = false;

  switch (feature_configs.config.featureType) {
    // if the feature type is point then just return true
    // coz preparesites has already done the conversion
    case VectorDefs::PointZ:
    case VectorDefs::IconZ:
      ret = true;
      break;

    case VectorDefs::LineZ:
      if ((*geode)->PrimType() == gstPolyLine) {
        if (feature_configs.config.drawAsRoads) {
          (*geode)->ChangePrimType(gstStreet);
        }
        ret = true;
      } else if ((*geode)->PrimType() == gstPolyLine25D) {
        if (feature_configs.config.drawAsRoads) {
          (*geode)->ChangePrimType(gstStreet25D);
        }
        ret = true;
      } else if ((*geode)->FlatPrimType() == gstStreet) {
        // gstStreet/gstStreet25D is Fusion internal type, we can't get it
        // from source data set.
        assert(feature_configs.config.drawAsRoads);
        ret = true;
      } else if ((*geode)->PrimType() == gstPolygon) {
        (*geode)->ChangePrimType(feature_configs.config.drawAsRoads ?
                                 gstStreet : gstPolyLine);
        ret = true;
      } else if ((*geode)->PrimType() == gstPolygon25D ||
                 (*geode)->PrimType() == gstPolygon3D) {
        (*geode)->ChangePrimType(feature_configs.config.drawAsRoads ?
                                 gstStreet25D : gstPolyLine25D);
        ret = true;
      } else if ((*geode)->PrimType() == gstMultiPolygon) {
        gstGeodeHandle new_geode;
        (*geode)->ChangePrimType(feature_configs.config.drawAsRoads ?
                                 gstStreet : gstPolyLine, &new_geode);
        (*geode).swap(new_geode);
        ret = true;
      } else if ((*geode)->PrimType() == gstMultiPolygon25D ||
                 (*geode)->PrimType() == gstMultiPolygon3D) {
        gstGeodeHandle new_geode;
        (*geode)->ChangePrimType(feature_configs.config.drawAsRoads ?
                                 gstStreet25D : gstPolyLine25D, &new_geode);
        (*geode).swap(new_geode);
        ret = true;
      }
      break;

    case VectorDefs::PolygonZ:
      if ((*geode)->PrimType() == gstPolyLine) {
        (*geode)->ChangePrimType(gstPolygon);
        ret = true;
      } else if ((*geode)->PrimType() == gstPolyLine25D) {
        (*geode)->ChangePrimType(gstPolygon25D);
        ret = true;
      } else if ((*geode)->PrimType() == gstPolygon ||
                 (*geode)->PrimType() == gstPolygon25D ||
                 (*geode)->PrimType() == gstPolygon3D ||
                 (*geode)->PrimType() == gstMultiPolygon ||
                 (*geode)->PrimType() == gstMultiPolygon25D ||
                 (*geode)->PrimType() == gstMultiPolygon3D) {
        ret = true;
      }
      break;
  }

  return ret;
}

#ifdef JOBSTATS_ENABLED
enum {
  JOBSTATS_PREP, JOBSTATS_INTERSECT, JOBSTATS_GETFEATURE,
  JOBSTATS_BOXCUT, JOBSTATS_GETLABEL, JOBSTATS_FETCH,
  JOBSTATS_EXPAND, JOBSTATS_REDUCE, JOBSTATS_REMOVEDUPS,
  JOBSTATS_JOIN, JOBSTATS_JOIN_GATHER, JOBSTATS_JOIN_WORK,
  JOBSTATS_JOIN_CLEAN, JOBSTATS_REMOVEINVAL, JOBSTATS_REDUCEROADS,
  JOBSTATS_SORTROADS, JOBSTATS_OVERLAP
};

static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_PREP,        "Prepare Features   "},
  {JOBSTATS_INTERSECT,   "+ Intersect        "},
  {JOBSTATS_GETFEATURE,  "+ Get Feature      "},
  {JOBSTATS_BOXCUT,      "+ Box Cut          "},
  {JOBSTATS_GETLABEL,    "+ Get Label        "},
  {JOBSTATS_FETCH,       "+ + Fetch          "},
  {JOBSTATS_EXPAND,      "+ + Expand         "},
  {JOBSTATS_REDUCE,      "+ Reduce           "},
  {JOBSTATS_REMOVEDUPS,  "+ + Remove Dups    "},
  {JOBSTATS_JOIN,        "+ + Join           "},
  {JOBSTATS_JOIN_GATHER, "+ + + Gather       "},
  {JOBSTATS_JOIN_WORK,   "+ + + Work         "},
  {JOBSTATS_JOIN_CLEAN,  "+ + + Clean        "},
  {JOBSTATS_REMOVEINVAL, "+ + Remove Invalid "},
  {JOBSTATS_REDUCEROADS, "+ + Reduce Roads   "},
  {JOBSTATS_SORTROADS,   "+ + + Sort         "},
  {JOBSTATS_OVERLAP,     "+ + + Rem Overlap  "}
};
gstJobStats* prep_stats = new gstJobStats("Prepare Features", JobNames, 17);
#endif

// PrepareFeatures() will iterate over all geodes contained in the filter
// identified by 'filter_id' and apply a box cutter with 'box'. The resulting
// pieces will be joined and reduced depending on the type, and then placed
// in the supplied vector 'feature_set'

bool gstSelector::PrepareFeatures(
    const gstQuadAddress& quad_address,
    int filter_id,
    const gstGeoIndexHandle &geo_index,
    const gstGeoIndexImpl::FeatureBucket &bucket,
    const gstGeoIndexImpl::FeatureList &featureList,
    gstFeatureSet *feature_set,
    JSDisplayBundle &jsbundle) {
  assert(gst_layer_ != NULL);
  try {
    JOBSTATS_SCOPED(prep_stats, JOBSTATS_PREP);
    // make some local handles for convenience
    GeodeList& geode_list = feature_set->glist;
    RecordList& label_list = feature_set->rlist;
    gstFilter* filter = GetFilter(filter_id);
    const gstFeatureConfigs &feature_configs = filter->FeatureConfigs();
    feature_set->feature_configs = &feature_configs;

    // Create BoxCutter and initialize with quad_address bounding box once.
    // Further use it many times.
    // true - cut_holes.
    fusion_gst::BoxCutter box_cutter(quad_address.bbox(), true);
#ifdef COMPLETELY_COVERED_OPT
    // Get whether quad is completely covered by any feature and the list of
    // feature Ids covering it. Stop cutting on features in quads that are
    // known to be fully contained within the feature (i.e. covered). Just
    // generate quad-size polygon of that feature.
    std::set<int> feature_id_set;
    bool quad_covered = geo_index->GetCovered(khTileAddr(quad_address.level(),
                                                         quad_address.row(),
                                                         quad_address.col()),
                                              &feature_id_set);
#endif

    for (gstGeoIndexImpl::FeatureBucketConstIterator subs = bucket.begin();
         subs != bucket.end(); ++subs) {
      int featureId = featureList[*subs].feature_id;

      JOBSTATS_BEGIN(prep_stats, JOBSTATS_GETFEATURE);
      gstGeodeHandle geode
        = theSourceManager->GetFeatureOrThrow(
            UniqueFeatureId(source_id_, layer_, featureId), false);
      JOBSTATS_END(prep_stats, JOBSTATS_GETFEATURE);

      if (!convertFeatureType(&geode, feature_configs)) {
        notify(
            NFY_FATAL,
            "Invalid conversion from resource type (%s) to display type (%s).",
            PrettyPrimType(geode->PrimType()).c_str(),
            feature_configs.PrettyFeatureType().ascii());
      }

      // cut out segments that fall in this quadnode(box)
      GeodeList pieces;

#ifdef COMPLETELY_COVERED_OPT
      if (quad_covered && feature_id_set.count(featureId) != 0) {
        // The quad is completely covered with current feature, so just
        // generate quad-size polygon.
        assert(!geode->IsDegenerate());
        double height25D = .0;
        const gstGeode *gd = NULL;
        if (geode->PrimType() == gstMultiPolygon ||
            geode->PrimType() == gstMultiPolygon25D ||
            geode->PrimType() == gstMultiPolygon3D) {
          const gstGeodeCollection *multi_geode =
              static_cast<const gstGeodeCollection*>(&(*geode));
          // Note: In case of multi-polygon features we do not
          // store index of polygon covering the quad with feature ID in
          // coverage mask. So, we just use first polygon from collection to
          // get height in case of it is 2.5D polygon.
          const gstGeodeHandle &geodeh_part0 = multi_geode->GetGeode(0);
          gd =
              static_cast<const gstGeode*>(&(*geodeh_part0));
        } else {
          gd = static_cast<const gstGeode*>(&(*geode));
        }
        assert(gd != NULL && !gd->IsDegenerate());
        height25D = gd->GetVertex(0, 0).z;
        gstGeodeHandle new_geodeh = gstGeodeImpl::Create(
            quad_address.bbox(), kClipEdge, gd->PrimType(), height25D);
        pieces.push_back(new_geodeh);
        gst_layer_->IncrementCovered(1);
      } else {
        bool completely_covered = false;
        JOBSTATS_SCOPED(prep_stats, JOBSTATS_BOXCUT);
        if (box_cutter.Run(geode, pieces, &completely_covered) == 0) {
          continue;
        }

        if (completely_covered) {
          // Update coverage mask.
          geo_index->SetCovered(khTileAddr(quad_address.level(),
                                           quad_address.row(),
                                           quad_address.col()),
                                featureId);
        }
      }
#else
      {
        JOBSTATS_SCOPED(prep_stats, JOBSTATS_BOXCUT);
        if (box_cutter.Run(geode, pieces) == 0) {
          continue;
        }
      }
#endif

      // assemble label record if needed
      gstRecordHandle label;
      JOBSTATS_BEGIN(prep_stats, JOBSTATS_GETLABEL);
      if (HasAttrib() && feature_configs.AttributeExpansionNeeded()) {
        JOBSTATS_BEGIN(prep_stats, JOBSTATS_FETCH);
        gstRecordHandle srcrec = theSourceManager->GetAttributeOrThrow(
            UniqueFeatureId(source_id_, layer_, featureId));
        JOBSTATS_END(prep_stats, JOBSTATS_FETCH);
        JOBSTATS_BEGIN(prep_stats, JOBSTATS_EXPAND);
        label = feature_configs.Expand(srcrec, jsbundle, filter_id);
        JOBSTATS_END(prep_stats, JOBSTATS_EXPAND);
      } else {
        label = feature_configs.DummyExpand();
      }

      if (!label) {
        // Expand error - almost certainly a JS error
        notify(NFY_FATAL, "Unable to compose feature label string");
      }

      // NOTE: After partitioning we do not have multi-polygon-features.
      // And all features contain only one part. It is checked in cycle below
      // and there is no additional checks further.

      // all pieces and labels get added together
      for (GeodeListIterator piece = pieces.begin();
           piece != pieces.end(); ++piece) {
        if ((*piece)->PrimType() == gstMultiPolygon ||
            (*piece)->PrimType() == gstMultiPolygon25D ||
            (*piece)->PrimType() == gstMultiPolygon3D ||
            (*piece)->NumParts() > 1) {
          notify(
              NFY_FATAL,
              "%s: Improper feature after partitioning(type: %d, parts: %d).",
              __func__, (*piece)->PrimType(), (*piece)->NumParts());
        }

        build_stats.total_vertexes += (*piece)->TotalVertexCount();
        geode_list.push_back(*piece);
        label_list.push_back(label);
      }
      JOBSTATS_END(prep_stats, JOBSTATS_GETLABEL);
    }

    // when geometry is cut by the box, it is very possible that nothing was
    // inside the box.  this is because up to this point, only the bounding
    // box of the geometry was used to determine intersection.  the cut will
    // compare every segment
    if (geode_list.empty()) {
      return false;
    }

    build_stats.total_features += geode_list.size();

    //
    // attempt to reduce unnecessary geometry
    //
    JOBSTATS_BEGIN(prep_stats, JOBSTATS_REDUCE);
    switch (feature_configs.config.featureType) {
      case VectorDefs::PointZ:
      case VectorDefs::IconZ:
        // do nothing
        break;
      case VectorDefs::LineZ:
        if (feature_configs.config.drawAsRoads) {
          // levels lower than 13 get overlapping segments removed
          // this is based on a tolerance derived from the level
          const bool remove_overlapping_segments = quad_address.level() <
              kAvoidRemovingOverlappingRoadSegmentsAtOrHigherZoomLevel;
          ReduceRoads(remove_overlapping_segments, quad_address.level(),
                      feature_set);
        } else {
          ReducePolylines(feature_set);
        }
        break;
      case VectorDefs::PolygonZ:
        ReducePolygons(feature_set);
        break;
    }
    JOBSTATS_END(prep_stats, JOBSTATS_REDUCE);

    build_stats.reduced_features += geode_list.size();
  } catch(const std::exception &e) {
    notify(NFY_FATAL,
           "Error while preparing features:\n"
           "%s\n"
           "Please check your filesystem.\n"
           "You may also want to rebuild the resource.",
           e.what());
  } catch(...) {
    notify(NFY_FATAL,
           "Error while preparing features:\n"
           "%s\n"
           "Please check your filesystem.\n"
           "You may also want to rebuild the resource.",
           "Unknown error");
  }

  return true;
}

void gstSelector::ReducePolygons(gstFeatureSet *fset) {
  build_stats.reduced_vertexes +=
    RemoveEmptyFeatures(&fset->glist, &fset->rlist, kMinCycleVertices);
}

void gstSelector::ReducePolylines(gstFeatureSet *fset) {
  // Reducing lines means reduce roads, but exclude overlapping segments filter
  // Pass level 0 to mean invalid level
  const bool remove_overlapping_segments = false;
  ReduceRoads(remove_overlapping_segments, 0, fset);
}

void gstSelector::ReduceRoads(const bool remove_overlapping_segments,
                              std::uint32_t level,
                              gstFeatureSet *fset) {
  JOBSTATS_SCOPED(prep_stats, JOBSTATS_REDUCEROADS);
  JOBSTATS_BEGIN(prep_stats, JOBSTATS_SORTROADS);
  //
  // sort all features by name
  //
  std::map<QString, GeodeList > name_map;
  GeodeListIterator geode = fset->glist.begin();
  RecordListIterator label = fset->rlist.begin();
  for (; geode != fset->glist.end(); ++geode, ++label)
    name_map[(*label)->Field(0)->ValueAsUnicode()].push_back(*geode);
  JOBSTATS_END(prep_stats, JOBSTATS_SORTROADS);

  //
  // join within each name set
  //
  for (std::map<QString, GeodeList>::iterator nameset =
         name_map.begin(); nameset != name_map.end(); ++nameset) {
    std::uint64_t num_duplicates = 0;
    std::uint64_t num_joined = 0;
    vectorprep::PolylineJoiner<GeodeList>::
        RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
            nameset->second, &num_duplicates, &num_joined);
    build_stats.duplicate_features += num_duplicates;
    build_stats.join_count += num_joined;
  }


  if (remove_overlapping_segments) {
    for (std::map<QString, GeodeList>::iterator nameset =
         name_map.begin(); nameset != name_map.end(); ++nameset) {
      JOBSTATS_BEGIN(prep_stats, JOBSTATS_REMOVEINVAL);
      GeodeList glist;
      for (GeodeListIterator g = nameset->second.begin();
          g != nameset->second.end(); ++g) {
        if (!(*g)->IsEmpty())
          glist.push_back(*g);
      }
      JOBSTATS_END(prep_stats, JOBSTATS_REMOVEINVAL);
      build_stats.overlap_count +=
        RemoveOverlappingSegments(&glist, level);
    }
  }

  build_stats.reduced_vertexes +=
    RemoveEmptyFeatures(&fset->glist, &fset->rlist, kMinPolylineVertices);
}


// Ensure all features are valid
// 1. make copies of geodes and records
// 2. clear out original lists
// 3. copy back valid features
unsigned int gstSelector::RemoveEmptyFeatures(
    GeodeList* geode_list,
    RecordList* record_list,
    unsigned int min_vertex_count) {
  JOBSTATS_SCOPED(prep_stats, JOBSTATS_REMOVEINVAL);
  if (geode_list->size() > record_list->size()) {
    geode_list->resize(record_list->size());
  } else if (geode_list->size() < record_list->size()) {
    record_list->resize(geode_list->size());
  }

  GeodeList orig_geode_list;
  orig_geode_list.swap(*geode_list);
  RecordList orig_record_list;
  orig_record_list.swap(*record_list);

  int valid_vertexes = 0;
  GeodeList::iterator geodeh = orig_geode_list.begin();
  RecordList::iterator record = orig_record_list.begin();
  for (; geodeh != orig_geode_list.end(); ++geodeh, ++record) {
    gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
    if (geode->NumParts() >= 1 &&
        geode->VertexCount(0) >= min_vertex_count) {
      valid_vertexes += geode->TotalVertexCount();
      // Using this swap technique rather than operator = we save on mutex calls
      // in khRefGuard.
      geode_list->push_back(gstGeodeHandle());
      geode_list->back().swap(*geodeh);
      record_list->push_back(gstRecordHandle());
      record_list->back().swap(*record);
    }
  }
  return valid_vertexes;
}

// TODO: duplicated in gst/vectorprep/DisplayRule.cpp
//
// extensive analysis by hand has resulted in
// choosing a constant of 28 to add to the level
// where we can remove overlapping segments
//
#define OVERLAP_CONSTANT 28

int gstSelector::RemoveOverlappingSegments(GeodeList* glist,
                                           int level) {
  JOBSTATS_SCOPED(prep_stats, JOBSTATS_OVERLAP);
  double epsilon = 1.0 / (1LLU << (level + OVERLAP_CONSTANT));

  int removed_count = 0;
  for (GeodeListIterator g1 = glist->begin();
      g1 != glist->end(); ++g1) {
    if ((*g1)->IsEmpty())
      continue;

    gstGeode *geode1 = static_cast<gstGeode*>(&(**g1));

    GeodeListIterator g2 = g1;
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

  return removed_count;
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_SITE_PREP = 0, JOBSTATS_SITE_INTERSECT,
      JOBSTATS_SITE_GETFEATURE, JOBSTATS_SITE_CALCLOC, JOBSTATS_SITE_GETLABEL,
      JOBSTATS_SITE_FETCH, JOBSTATS_SITE_EXPAND, JOBSTATS_SITE_REMOVEDUPS};

static gstJobStats::JobName SiteJobNames[] = {
  {JOBSTATS_SITE_PREP,        "Total              "},
  {JOBSTATS_SITE_INTERSECT,   "+ Intersect        "},
  {JOBSTATS_SITE_GETFEATURE,  "+ GetFeature       "},
  {JOBSTATS_SITE_CALCLOC,     "+ Calc Location    "},
  {JOBSTATS_SITE_GETLABEL,    "+ Get Label        "},
  {JOBSTATS_SITE_FETCH,       "+ + Fetch          "},
  {JOBSTATS_SITE_EXPAND,      "+ + Expand         "},
  {JOBSTATS_SITE_REMOVEDUPS,  "+ Remove Dups      "},
};
gstJobStats* prep_site_stats =
    new gstJobStats("Prepare Sites", SiteJobNames, 8);
#endif

#undef DUMP_QUADS

bool gstSelector::PrepareSites(const gstQuadAddress& quad_address,
                               int filter_id,
                               const gstGeoIndexImpl::FeatureBucket &bucket,
                               const gstGeoIndexImpl::FeatureList &featureList,
                               gstSiteSet& site_set,
                               JSDisplayBundle &jsbundle) {
  if (!HasAttrib()) {
    // neither the old record expansion nor the new JS expansion can deal
    // with not having records. This means we cannot generate sites where
    // the label and popup text is contant (i.e. doesn't depend on the
    // attribute data). But that's not such a bad restriction. Is it?
    return false;
  }

  try {
    JOBSTATS_SCOPED(prep_site_stats, JOBSTATS_SITE_PREP);
    VertexList& vlist = site_set.vlist;
    RecordList& rlist = site_set.rlist;

    gstFilter* filter = GetFilter(filter_id);
    site_set.site = &filter->Site();

#ifdef DUMP_QUADS
    std::vector<int> hits;
    hits.reserve(bucket.size());
#endif
    for (gstGeoIndexImpl::FeatureBucketConstIterator it = bucket.begin();
         it != bucket.end(); ++it) {
      int featureId = featureList[*it].feature_id;

      JOBSTATS_BEGIN(prep_site_stats, JOBSTATS_SITE_GETFEATURE);
      gstGeodeHandle geode = theSourceManager->GetFeatureOrThrow(
          UniqueFeatureId(source_id_, layer_, featureId), false);
      JOBSTATS_END(prep_site_stats, JOBSTATS_SITE_GETFEATURE);

      // determine the label location for this feature
      JOBSTATS_BEGIN(prep_site_stats, JOBSTATS_SITE_CALCLOC);
      gstVertex v = filter->Site().Location(geode);
      JOBSTATS_END(prep_site_stats, JOBSTATS_SITE_CALCLOC);

      // discard if it falls outside our quad
      if (!quad_address.bbox().Contains(v))
        continue;

      {
        JOBSTATS_SCOPED(prep_site_stats, JOBSTATS_SITE_GETLABEL);

        JOBSTATS_BEGIN(prep_site_stats, JOBSTATS_SITE_FETCH);
        // we've already exited if we don't have attributes
        gstRecordHandle rec = theSourceManager->GetAttributeOrThrow(
              UniqueFeatureId(source_id_, layer_, featureId));
        JOBSTATS_END(prep_site_stats, JOBSTATS_SITE_FETCH);
        if (rec->IsEmpty()) {
          notify(NFY_WARN, "Empty record!  src:%d layer:%d feature:%d",
                 source_id_, layer_, featureId);
          continue;
        }

        JOBSTATS_BEGIN(prep_site_stats, JOBSTATS_SITE_EXPAND);
        gstRecordHandle expanded =
          filter->Site().Expand(rec, false /* labelOnly */,
                                jsbundle, filter_id);
        JOBSTATS_END(prep_site_stats, JOBSTATS_SITE_EXPAND);

        if (!expanded) {
          // Expand error - almost certainly a JS error
          // Error message already emitted
          return false;
        }
        rlist.push_back(expanded);
      }
      vlist.push_back(v);
#ifdef DUMP_QUADS
      hits.push_back(featureId);
#endif
    }

    if (vlist.size() == 0)
      return false;

#ifdef DUMP_QUADS
    fprintf(stderr, "QUAD:%u,%u,%u:%d:",
            quad_address.level(), quad_address.row(), quad_address.col(),
            filter_id);
    for (unsigned int i = 0; i < hits.size(); ++i) {
      fprintf(stderr, "%u,", hits[i]);
    }
    fprintf(stderr, "\n");
#endif

    build_stats.total_features += vlist.size();
    build_stats.reduced_features += vlist.size();
    build_stats.total_vertexes += vlist.size();
    build_stats.reduced_vertexes += vlist.size();

    // Remove duplicate sites.
    JOBSTATS_BEGIN(prep_site_stats, JOBSTATS_SITE_REMOVEDUPS);
    if (site_set.site->config.suppressDuplicateSites)
      build_stats.duplicate_sites += RemoveDuplicateSites(
          &site_set.vlist, &site_set.rlist);
    JOBSTATS_END(prep_site_stats, JOBSTATS_SITE_REMOVEDUPS);
  } catch(const std::exception &e) {
    notify(NFY_FATAL,
           "Error while preparing sites:\n"
           "%s\n"
           "Please check your filesystem.\n"
           "You may also want to rebuild the resource.",
           e.what());
  } catch(...) {
    notify(NFY_FATAL,
           "Error while preparing sites:\n"
           "%s\n"
           "Please check your filesystem.\n"
           "You may also want to rebuild the resource.",
           "Unknown error");
  }

  return true;
}

// Used only by GUI
bool gstSelector::PrepareSiteLabels(const gstBBox& box,
                                    int filter_id,
                                    gstSiteSet& site_set,
                                    JSDisplayBundle &jsbundle,
                                    bool is_mercator_preview) {
  if (!HasAttrib()) {
    return false;
  }

  // we shouldn't have to worry about failed feature or attribute fetches
  // since the GUI would already have found them while applying the filter.
  // But just in case, we put a try block around the whole routine.
  // This is called from the Draw function where we really shouldn't
  // open a dialog. Since the errors should already have been caught, and we
  // can't report from here anyway, we'll just return false
  try {
    VertexList& vlist = site_set.vlist;
    RecordList& rlist = site_set.rlist;

    gstFilter* filter = GetFilter(filter_id);
    site_set.site = &filter->Site();

    // perform a geospatial select on the filter with this quad geocoords
    std::vector<int> subset;
    if (is_mercator_preview) {
      // In this case box is mercator normalized cullbox(feature bounding box).
      // Need to use Flat Projection normalized cullbox for
      // filter->IntersectBBox as filter bounding box is still in
      // Flat Projection normalized coordinates.
      gstBBox cull_box = box;
      MercatorProjection::NormalizeToFlatFromMercator(&cull_box.n, &cull_box.s);
      filter->IntersectBBox(cull_box, &subset);
    } else {
      filter->IntersectBBox(box, &subset);
    }

    for (std::vector<int>::const_iterator it = subset.begin();
         it != subset.end(); ++it) {
      gstGeodeHandle geode = theSourceManager->GetFeatureOrThrow(
          UniqueFeatureId(source_id_, layer_, *it), is_mercator_preview);

      // determine the label location for this feature
      gstVertex v = filter->Site().Location(geode);

      // discard if it falls outside our quad
      if (!box.Contains(v))
        continue;

      gstRecordHandle rec = theSourceManager->GetAttributeOrThrow(
          UniqueFeatureId(source_id_, layer_, *it));
      if (rec->IsEmpty()) {
        notify(NFY_WARN, "Empty record!  src:%d layer:%d feature:%d",
               source_id_, layer_, *it);
        continue;
      }

      gstRecordHandle expanded =
        filter->Site().Expand(rec, true /* filterOnly */,
                              jsbundle, filter_id);
      if (!expanded) {
        // Expand error - almost certainly a JS error
        // Error message already emitted
        return false;
      }
      rlist.push_back(expanded);
      vlist.push_back(v);
    }

    return true;
  } catch(...) {
    // silently ignore. see comment before 'try'
  }

  return false;
}

class GstValueHandle {
 public:
  explicit GstValueHandle(const gstValue* ptr)
      : handle_(ptr) {}
  bool operator<(const GstValueHandle& other) const {
    return (*handle_ < *other.handle_);
  }
 private:
  const gstValue* const handle_;
};

// Remove any duplicate sites from the given site set.  A duplicate is
// defined to be a site with exactly the same position and the same name.
// Unfortunately many of our data sets (eg lodging) contain such duplicates
// and this is a reasonable place to remove them.
int gstSelector::RemoveDuplicateSites(VertexList* vlist_ptr,
                                      RecordList* rlist_ptr) {
  assert(vlist_ptr->size() == rlist_ptr->size());
  std::vector<size_t> duplicates_at_indices;
  {
    VertexList& vlist = *vlist_ptr;
    RecordList& rlist = *rlist_ptr;
    std::set<std::pair<gstVertex, GstValueHandle> > existing_position_names;
    for (unsigned int i = 0; i < vlist.size(); ++i) {
      const bool is_new_position_name = existing_position_names.insert(
          std::make_pair(vlist[i], GstValueHandle(rlist[i]->Field(0)))).second;
      if (!is_new_position_name) {  // This tuple (vertex, name) is not unique
        duplicates_at_indices.push_back(i);
      }
    }
  }
  if (!duplicates_at_indices.empty()) {
    RemoveAtIndices<std::vector<size_t>,
        std::vector<size_t>::const_iterator, VertexList, VertexList::iterator>(
        duplicates_at_indices, vlist_ptr);
    RemoveAtIndicesUsingSwap<std::vector<size_t>,
        std::vector<size_t>::const_iterator, RecordList, RecordList::iterator>(
        duplicates_at_indices, rlist_ptr);
  }
  return duplicates_at_indices.size();
}


void gstSelector::deselect(int* rows, int count, bool selected) {
  // build a list of each geode based on the row #
  // the row # will match the child #

#if 0
  khArray<gstGeode*> geodes(count);

  for (int r = 0; r < count; r++)
    geodes.append(static_cast<gstGeode*>(pick_list_->child(rows[r])));

  if (selected) {
    for (int r = 0; r < count; r++)
      pick_list_->removeChild(geodes[r]);
  } else {
    pick_list_->clear();
    for (int r = 0; r < count; r++)
      pick_list_->addChild(geodes[r]);
  }
#endif
}

 std::uint32_t gstSelector::NumSrcFeatures(void) const {
  return getSource()->NumFeatures(layer_);
}

double gstSelector::AverageSrcFeatureDiameter(void) const {
  return getSource()->AverageFeatureDiameter(layer_);
}

// This routine is only called by gevectorquery
void gstSelector::ThrowingApplyQueries(khProgressMeter *logProgress) {
  // Don't allow any soft errors - make them all fatal
  SoftErrorPolicy soft_error_policy(0);
  ThrowingApplyQueries(0 /* guiProgress */,
                       soft_error_policy,
                       logProgress);
}

// step through every geode of source file and apply filters
// sequentially.
void gstSelector::ThrowingApplyQueries(gstProgress* guiProgress,
                                       SoftErrorPolicy &soft_error_policy,
                                       khProgressMeter *logProgress) {
  query_complete_ = false;

  if (guiProgress)
    guiProgress->SetVal(0);

  // figure out if we have JS expressions for any of our filters
  bool wantJS = false;
  for (unsigned int f = 0; f < config.filters.size(); ++f) {
    if (config.filters[f].match == FilterConfig::JSExpression) {
      wantJS = true;
      break;
    }
  }

  // always initialize filter matches first
  for (unsigned int f = 0; f < NumFilters(); ++f)
    GetFilter(f)->Reset();

  // picklist cannot be valid if there are no filters!
  pick_list_.clear();

  //---------------------------------------------------------------------------
  // iterate through every geode of source file, comparing with
  // each filter sequentially
  //---------------------------------------------------------------------------
  gstSource* src = getSource();

  if (src == NULL) {
    throw khException(kh::tr("Unable to retrieve source"));
  }

  int gnum = src->NumFeatures(layer_);
  if (gnum == 0) {
    throw khException(kh::tr("Source %1 appears to be empty or corrupt.\n"
                             "Feature count is 0.")
                      .arg(src->name()));
  }



  // create the JS context and compile the match scripts
  gstHeaderHandle recordHeader = src->GetAttrDefs(layer_);
  gstRecordJSContext cx;
  std::vector<KHJSScript> matchScripts(NumFilters());
  if (wantJS) {
    try {
      // my config's context script will already have all of the levels of
      // context scripts collapsed into it.
      QStringList contextScripts;
      if (!config.contextScript.isEmpty()) {
        contextScripts.append(config.contextScript);
      }
      cx = gstRecordJSContextImpl::Create(recordHeader, contextScripts);
      for (unsigned int f = 0; f < config.filters.size(); ++f) {
        if (config.filters[f].match == FilterConfig::JSExpression) {
          matchScripts[f] = cx->CompileScript(config.filters[f].matchScript,
                                              QString("Filter Expression %1")
                                              .arg(f+1));
        }
      }
    } catch(const khException &e) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg(e.qwhat()));
    } catch(const std::exception &e) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg(e.what()));
    } catch(...) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg("Unknown"));
    }
  }

  double step = 100.0 / static_cast<double>(gnum);

  notify(NFY_DEBUG, "Source %s has %d features for layer %d",
         src->name(), gnum, layer_);
  khTimer_t start = khTimer::tick();

  notify(NFY_PROGRESS, "Query %d features on layer %d", gnum, layer_);

  khTimer_t begin_query = khTimer::tick();
  const int kCountInterval = 100000;
  QString error;
  for (int fnum = 0; fnum < gnum; ++fnum) {
    //
    // has the user tried to interrupt?
    //
    if (guiProgress && (guiProgress->GetState() == gstProgress::Interrupted)) {
      return;
    }

    // if we have any fields, we need to fetch the record
    gstRecordHandle recordHandle;
    if (recordHeader->numColumns() > 0) {
      try {
        recordHandle = src->GetAttributeOrThrow(layer_, fnum);
      } catch(const khSoftException &e) {
        soft_error_policy.HandleSoftError(e.qwhat());
        continue;
      }
    }

    if (cx) {
      cx->SetRecord(recordHandle);
    }

    for (unsigned int f = 0; f < NumFilters(); ++f) {
      bool match = false;
      GetFilter(f)->ThrowingTryApply(recordHandle,
                                     UniqueFeatureId(source_id_, layer_, fnum),
                                     cx,  matchScripts[f],
                                     match, soft_error_policy);

      // see if features can be shared across filters
      if (match && !config.allowFeatureDuplication) {
        break;
      }
    }

    if (guiProgress)
      guiProgress->SetVal(static_cast<int>(static_cast<double>(fnum) * step));

    if (logProgress)
      logProgress->increment();

    if (((fnum + 1) % kCountInterval == 0) && (fnum != 0)) {
      if (getNotifyLevel() >= NFY_PROGRESS) {
        notify(NFY_PROGRESS,
               "Query features (%d-%d) took %.4lf secs, est total mins: %.2lf",
               (fnum - kCountInterval + 1), fnum,
               khTimer::delta_s(begin_query, khTimer::tick()),
               (((khTimer::delta_s(start, khTimer::tick()) / fnum) * gnum) /
                60.0));
        notify(NFY_PROGRESS, "v:%d s:%d i:%d d:%d\n",
               gstValue::vcount, gstValue::scount,
               gstValue::icount, gstValue::dcount);
        JOBSTATS_DUMPALL();
        notify(NFY_PROGRESS, "#####################");

        begin_query = khTimer::tick();
      }
    }
  }

  double queryTime = khTimer::delta_s(start, khTimer::tick());

  notify(NFY_DEBUG, "Total query time=%.2lf seconds", queryTime);

  if (guiProgress)
    guiProgress->SetVal(100);

  // after queries are complete, rebuild geoindex
  for (unsigned int f = 0; f < NumFilters(); ++f)
    GetFilter(f)->Finalize();

  query_complete_ = true;
}


// This routine is only called by gevectorquery
void gstSelector::CreateSelectionListFilesBatch(
  const std::string& filter_selection_name_prefix,
  khProgressMeter *logProgress) {
  // Don't allow any soft errors - make them all fatal
  SoftErrorPolicy soft_error_policy(0);
  CreateSelectionListFilesBatch(filter_selection_name_prefix,
                       0 /* guiProgress */,
                       soft_error_policy,
                       logProgress);
}

// Local utility "struct" used by CreateSelectionListFilesBatch.
class FeatureInfo {
 public:
  UniqueFeatureId id_;
  gstRecordHandle record_;
  gstBBox box_;

  FeatureInfo() : id_(0, 0, 0) {}

  FeatureInfo(const UniqueFeatureId& id, const gstRecordHandle& record,
              const gstBBox& box) : id_(0, 0, 0) {
    id_ = id;
    record_ = record;
    box_ = box;
  }
};

// Local utility "struct" used by CreateSelectionListFilesBatch.
class FilterInfo {
 public:
  std::vector<int> selections_;
  gstBBox box_;
  unsigned int select_count_;
  std::string path_;
  FILE* file_;
  FilterInfo() : select_count_(0), file_(NULL) {}
};

// Step through every geode of source file and apply filters
// sequentially, writing out the selected feature ids as we go.
// This code is optimized to keep the memory footprint low and reduce I/O calls
// (both for reading the features and writing the results).
void gstSelector::CreateSelectionListFilesBatch(
    const std::string& filter_selection_name_prefix,
    gstProgress* guiProgress, SoftErrorPolicy &soft_error_policy,
    khProgressMeter *logProgress) {
  query_complete_ = false;
  bool do_not_allow_feature_duplication = !config.allowFeatureDuplication;

  if (guiProgress)
    guiProgress->SetVal(0);

  // figure out if we have JS expressions for any of our filters
  bool wantJS = false;
  for (unsigned int filter_index = 0;
       filter_index < config.filters.size(); ++filter_index) {
    if (config.filters[filter_index].match == FilterConfig::JSExpression) {
      wantJS = true;
      break;
    }
  }

  // always initialize filter matches first
  for (unsigned int filter_index = 0; filter_index < NumFilters(); ++filter_index)
    GetFilter(filter_index)->Reset();

  // picklist cannot be valid if there are no filters!
  pick_list_.clear();

  //---------------------------------------------------------------------------
  // iterate through every geode of source file, comparing with
  // each filter sequentially
  //---------------------------------------------------------------------------
  gstSource* gst_source = getSource();

  if (gst_source == NULL) {
    throw khException(kh::tr("Unable to retrieve source"));
  }

  int feature_count = gst_source->NumFeatures(layer_);
  if (feature_count == 0) {
    throw khException(kh::tr("Source %1 appears to be empty or corrupt.\n"
                             "Feature count is 0.")
                      .arg(gst_source->name()));
  }



  // create the JS context and compile the match scripts
  gstHeaderHandle recordHeader = gst_source->GetAttrDefs(layer_);
  if (recordHeader->numColumns() == 0) {
    throw khException(kh::tr("Source %1 appears to be empty or corrupt.\n"
                             "Attribute count is 0.")
                      .arg(gst_source->name()));
  }

  gstRecordJSContext record_js_context;
  std::vector<KHJSScript> matchScripts(NumFilters());
  if (wantJS) {
    try {
      // my config's context script will already have all of the levels of
      // context scripts collapsed into it.
      QStringList contextScripts;
      if (!config.contextScript.isEmpty()) {
        contextScripts.append(config.contextScript);
      }
      record_js_context =
          gstRecordJSContextImpl::Create(recordHeader, contextScripts);
      for (unsigned int filter_index = 0;
           filter_index < config.filters.size(); ++filter_index) {
        if (config.filters[filter_index].match == FilterConfig::JSExpression) {
          matchScripts[filter_index] =
              record_js_context->CompileScript(
                  config.filters[filter_index].matchScript,
                  QString("Filter Expression %1").arg(filter_index+1));
        }
      }
    } catch(const khException &e) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg(e.qwhat()));
    } catch(const std::exception &e) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg(e.what()));
    } catch(...) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg("Unknown"));
    }
  }

  double progress_step = 100.0 / static_cast<double>(feature_count);

  notify(NFY_DEBUG, "Source %s has %d features for layer %d",
         gst_source->name(), feature_count, layer_);
  khTimer_t start = khTimer::tick();

  notify(NFY_PROGRESS, "Query %d features on layer %d", feature_count, layer_);

  khTimer_t begin_query = khTimer::tick();
  QString error;

  /*
    init M filter extents and filter files
    for each 1M features in N features
    vector<bool> valid_features = vector<bool>(count, true);
    vector<box> feature_boxes = vector<box>(count, true);
    for each feature
    boxes[i] = box
    valid_features = box.Valid();
    for each filter
    id_list.clear()
    for each feature
    if valid_feature[i]
    if (filter->eval(feature[i]))
    push_back(id)
    filter_box[j].Union(feature_boxes[i]);
    append to feature ids filter file
    prepend extents to filter file
  */
  const int kBatchCount = 1000;
  const int kBatchesDumpStatsInterval = 100;
  int batch_count = 0;
  std::vector<FeatureInfo> features;
  std::vector<FilterInfo> filters(NumFilters());

  // We'll have a string buffer for the ascii feature id's for each filter.
  // Reserve the space once and reuse for all
  // 11 decimal digits plus newline should be plenty for us.
  const int kFeatureIdCharCount = 12;
  const char newline = '\n';
  std::string feature_id_buffer;
  feature_id_buffer.reserve(kBatchCount * kFeatureIdCharCount);

  // Reserve plenty of space for 4 25-digit numbers so that we can write to the
  // select files and prepend the extents at the end after they've been
  // computed.
  static const int kInitialBufferSize = 256;
  // Additional box to check for feature validity.
  static const gstBBox kMaxDomain(0.0, 1.0, 0.25, 0.75);

  // Create and open the filter result files.
  for (unsigned int filter_index = 0; filter_index < NumFilters(); ++filter_index) {
    FilterInfo& filter_info = filters[filter_index];

    // space for ".00" + NULL
    const size_t kPathSize = filter_selection_name_prefix.size() + 6;
    char path[kPathSize];

    snprintf(path, kPathSize, "%s.%02d",
            filter_selection_name_prefix.c_str(), filter_index);

    // silently remove file if it already exists
    if (khExists(path)) {
      if (unlink(path) == -1) {
        notify(NFY_FATAL, "Unable to successfully create filter results\n"
               "Unable to remove select results file %s", path);
      }
    }

    FILE* file = fopen(path, "w");
    if (file == NULL) {
      notify(NFY_FATAL, "Unable to successfully create filter results\n"
             "Unable to create select results file %s", path);
    }
    filter_info.file_ = file;
    filter_info.path_ = path;

    // We need to pad the beginning of the file with whitespace line so that we
    // can efficiently prepend the results of the extents after all features
    // have been filtered.
    for (int i = 0; i < kInitialBufferSize-1; ++i) {
      fputc(' ', file);
    }
    fputc('\n', file);
  }

  // Reserve the space beforehand!
  features.reserve(kBatchCount);
  for (unsigned int filter_index = 0; filter_index < NumFilters(); ++filter_index) {
    filters[filter_index].selections_.reserve(kBatchCount);
  }

  // Walk through the geodes and process them in groups of kBatchCount
  // Ideas is to maximize I/O and limit memory usage for large vector sets.
  int current_feature_id_base = 0;
  while (current_feature_id_base < feature_count) {
    // Has the user tried to interrupt?
    if (guiProgress && (guiProgress->GetState() == gstProgress::Interrupted)) {
      return;
    }

    // Load the next "current_batch_count" geodes. Don't skip past the end.
    int current_batch_count =
        std::min((feature_count - current_feature_id_base), kBatchCount);

    // Clear the "Batch" vectors.
    features.clear();
    for (unsigned int filter_index = 0; filter_index < NumFilters(); ++filter_index) {
      filters[filter_index].selections_.clear();
    }

    // Begin this "Batch" by loading all the geodes and checking validity.
    // TODO: fix to read the next "current_batch_count" geodes in one read!
    // temporarily load one-by-one.
    for (int i = 0; i < current_batch_count; ++i) {
      int feature_index = current_feature_id_base + i;
      try {
        // Fetch the record
        gstRecordHandle recordHandle =
            gst_source->GetAttributeOrThrow(layer_, feature_index);

        // Get the bounding box and check validity.
        gstBBox box = gst_source->GetFeatureBoxOrThrow(layer_, feature_index);
        if (!box.Valid()) {
          throw khSoftException(kh::tr("Invalid bounding box for feature %1")
                                .arg(feature_index));
        }
        if (!kMaxDomain.Contains(box)) {
          throw khSoftException(QString().sprintf(
              "rejecting invalid feature (%d)"
              "(n:%6.6lg s:%6.6lg w:%6.6lg e:%6.6lg)",
              feature_index,
              khTilespace::Denormalize(box.n),
              khTilespace::Denormalize(box.s),
              khTilespace::Denormalize(box.w),
              khTilespace::Denormalize(box.e)));
        }
        // Only push valid features onto the Batch vectors.
        features.push_back(FeatureInfo(
            UniqueFeatureId(source_id_, layer_, feature_index),
            recordHandle, box));
      } catch(const khSoftException &e) {
        // Handle the soft error. At least the feature will be marked invalid.
        soft_error_policy.HandleSoftError(e.qwhat());
      }
    }

    // Process each geode in this batch.
    // Note: there may be less than "current_batch_count" features at this
    // point since invalid features have been stripped out.
    for (unsigned int i = 0; i < features.size(); ++i) {
      UniqueFeatureId& unique_feature_id = features[i].id_;
      gstRecordHandle recordHandle = features[i].record_;

      if (record_js_context) {
        record_js_context->SetRecord(recordHandle);
      }

      for (unsigned int filter_index = 0; filter_index < NumFilters(); ++filter_index) {
        FilterInfo& filter_info = filters[filter_index];
        bool has_match = GetFilter(filter_index)->ThrowingTryHasMatch(
            recordHandle, unique_feature_id, record_js_context,
            matchScripts[filter_index], soft_error_policy);
        if (has_match) {
          filter_info.selections_.push_back(unique_feature_id.feature_id);
          filter_info.box_.Grow(features[i].box_);
          filter_info.select_count_++;
          // Check if features can be shared across filters.
          if (do_not_allow_feature_duplication) {
            break;
          }
        }
      }
    }

    // Append all filter_selections so far to the appropriate filter file.
    for (unsigned int filter_index = 0; filter_index < NumFilters(); ++filter_index) {
      FilterInfo& filter_info = filters[filter_index];
      std::vector<int>& filter_selection = filter_info.selections_;

      if (filter_selection.empty()) {
        continue;
      }
      // Must write out ASCII newline separated list of selected feature
      // indices. Use a preallocated string buffer to buffer up into a
      // single write call.
      feature_id_buffer.clear();
      for (unsigned int i = 0; i < filter_selection.size(); ++i) {
        feature_id_buffer.append(Itoa(filter_selection[i]));
        feature_id_buffer.append(1, newline);
      }
      FILE* file = filter_info.file_;
      fwrite(feature_id_buffer.data(), 1, feature_id_buffer.size(), file);
    }

    // Increment current_feature_id_base
    current_feature_id_base += current_batch_count;

    // Update Progress.
    if (guiProgress)
      guiProgress->SetVal(static_cast<int>(
          static_cast<double>(current_feature_id_base) * progress_step));
    if (logProgress)
      logProgress->incrementDone(current_batch_count);

    // Dump Stats every kBatchesDumpStatsInterval batches.
    batch_count++;
    if (batch_count % kBatchesDumpStatsInterval == 0) {
      if (getNotifyLevel() >= NFY_PROGRESS) {
        notify(NFY_PROGRESS,
               "Query features (%d-%d) took %.4lf secs, est total mins: %.2lf",
               (current_feature_id_base - current_batch_count),
               (current_feature_id_base - 1),
               khTimer::delta_s(begin_query, khTimer::tick()),
               (((khTimer::delta_s(start, khTimer::tick()) /
                  current_feature_id_base) * feature_count) / 60.0));
        notify(NFY_PROGRESS, "v:%d s:%d i:%d d:%d\n",
               gstValue::vcount, gstValue::scount,
               gstValue::icount, gstValue::dcount);
        JOBSTATS_DUMPALL();
        notify(NFY_PROGRESS, "#####################");

        // Set timer for next stats dump.
        begin_query = khTimer::tick();
      }
    }
  }

  // Prepend the extents to the selection files.
  for (unsigned int filter_index = 0; filter_index < NumFilters(); ++filter_index) {
    FilterInfo& filter_info = filters[filter_index];
    FILE* file = filter_info.file_;

    // Remove empty selection files (no file is written if their count is 0).
    if (filter_info.select_count_ == 0) {
      fclose(file);
      // must delete the file.
      unlink(filter_info.path_.c_str());
      continue;
    }

    // Prepend the extents for non-empty selections
    // Note that we reserved the the space for this when we created the file.
    gstBBox& box = filter_info.box_;
    char buffer[kInitialBufferSize];  // plenty of space for 4 25-long numbers.
    snprintf(buffer, kInitialBufferSize,
             "EXTENTS: %.20lf, %.20lf, %.20lf, %.20lf",
             box.w, box.e, box.s, box.n);

    // Write out the buffer to the file...room was reserved when the file
    // was created. Then close and cleanup the file.
    filter_info.file_ = NULL;  // Clear for safety. We're done with it.
    fseek(file, 0, 0);
    fwrite(buffer, 1, strlen(buffer), file);
    fclose(file);
  }

  // Print the time taken for the query.
  double queryTime = khTimer::delta_s(start, khTimer::tick());
  notify(NFY_DEBUG, "Total query time=%.2lf seconds", queryTime);

  if (guiProgress)
    guiProgress->SetVal(100);

  query_complete_ = true;
}


void gstSelector::drawFeatures(int* max_count, const gstDrawState& state) {
  for (unsigned int f = 0; f < NumFilters(); ++f) {
    gstFilter* filter = GetFilter(f);

    if (!filter->IsVisible(state))
      continue;

    filter->DrawSelectedFeatures(state, source_id_, layer_, max_count);
    if (*max_count <= 0)
      break;
  }
}

void gstSelector::drawSelectedFeatures(const gstDrawState& state) {
  gstDrawState enhancedState = state;
  enhancedState.mode |= DrawEnhanced;
  enhancedState.mode |= DrawSelected;

  // We don't have to worry about GetFeature failing since we only call it
  // for features that are already in our picklist. And since we're drawing
  // we cannot open a dialog anyway
  try {
    for (unsigned int fnum = 0; fnum < pick_list_.size(); ++fnum) {
      gstGeodeHandle geode
        = theSourceManager->GetFeatureOrThrow(
            UniqueFeatureId(source_id_, layer_, pick_list_[fnum]),
                            state.IsMercatorPreview());

      if (state.cullbox.Intersect(geode->BoundingBox())) {
        geode->Draw(enhancedState, gstFeaturePreviewConfig());
      }
    }
  } catch(...) {
    // silently ignore. see comment before 'try'
  }
}

void gstSelector::RemoveAllFilters() {
  for (std::vector<gstFilter*>::iterator it = filters_.begin();
       it != filters_.end(); ++it) {
    delete *it;
  }
  filters_.clear();
}

class SortItem {
 public:
  SortItem() : item(NULL), index(0) {}
  ~SortItem(void) { delete item; }

  void set(int id, gstValue* i) {
    index = id;
    item = new gstValue(*i);
  }
  void set(int id) { index = id; }

  int getIndex() const { return index; }
  gstValue* getItem() const { return item; }

 private:
  gstValue* item;
  int index;
};

extern "C" {
  static int cmpItems(const void* n1, const void* n2) {
    if (!n1 || !n2)
      return 0;

    const SortItem* i1 = reinterpret_cast<const SortItem*>(n1);
    const SortItem* i2 = reinterpret_cast<const SortItem*>(n2);

    return i1->getItem()->compare(*(i2->getItem()));
  }

  static int cmpIndexes(const void* n1, const void* n2) {
    if (!n1 || !n2)
      return 0;

    const SortItem* i1 = reinterpret_cast<const SortItem*>(n1);
    const SortItem* i2 = reinterpret_cast<const SortItem*>(n2);

    return i1->getIndex() - i2->getIndex();
  }
}

//
// column 0 is the row index
//
const SelectList& gstSelector::sortColumnOrThrow(int col, bool ascending) {
  if (!HasAttrib()) {
    return pick_list_;
  }

  if (uint(col) > getSource()->GetAttrDefs(layer_)->numColumns())
    return pick_list_;

  if (!pick_list_.size())
    return pick_list_;

  // construct an array of all fields that need sorting
  SortItem* items = new SortItem[pick_list_.size()];
  khDeleteGuard<SortItem, ArrayDeleter> deleter(TransferOwnership(items));

  if (col == 0) {
    for (unsigned int n = 0; n < pick_list_.size(); ++n) {
      items[n].set(pick_list_[n]);
    }
    qsort(items, pick_list_.size(), sizeof(SortItem), cmpIndexes);
  } else {
    for (unsigned int n = 0; n < pick_list_.size(); ++n) {
      gstRecordHandle src_rec = theSourceManager->GetAttributeOrThrow(
          UniqueFeatureId(source_id_, layer_, pick_list_[n]));
      items[n].set(pick_list_[n], src_rec->Field(col - 1));
    }

    qsort(items, pick_list_.size(), sizeof(SortItem), cmpItems);
  }

  SelectList sorted;
  sorted.reserve(pick_list_.size());

  if (ascending) {
    for (unsigned int n = 0; n < pick_list_.size(); ++n) {
      sorted.push_back(items[n].getIndex());
    }
  } else {
    unsigned int sz = pick_list_.size();
    for (unsigned int n = 0; n < pick_list_.size(); ++n) {
      sorted.push_back(items[sz - n - 1].getIndex());
    }
  }

  pick_list_ = sorted;

  return pick_list_;
}


bool gstSelector::HasAttrib(void) const {
  gstSource *src = getSource();
  if (!src->Opened()) {
    // ignore the error on this delayed open. We really cannot do anything
    // about it and somebodly else will recognize the failure later on
    (void) src->Open();
  }

  return src->Opened() ? src->HasAttrib(layer_) : false;
}
