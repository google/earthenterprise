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


#include "fusion/gst/gstLayer.h"

#include <unistd.h>
#include <limits.h>
#include <GL/gl.h>

#include <cmath>
#include <algorithm>

#include <qdatetime.h>
#include <qstringlist.h>
#include <qimage.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdir.h>

#include "common/khInsetCoverage.h"
#include "common/khHashTable.h"
#include "common/khGuard.h"
#include "common/khProgressMeter.h"
#include "common/khTileAddr.h"
#include "fusion/gst/gstExport.h"
#include "fusion/gst/gstFilter.h"
#include "fusion/gst/gstSelector.h"
#include "fusion/gst/gstBBox.h"
#include "fusion/gst/gstVectorProject.h"
#include "fusion/gst/gstSource.h"
#include "fusion/gst/gstFormat.h"
#include "fusion/gst/gstQuadAddress.h"
#include "fusion/gst/gstFeature.h"
#include "fusion/gst/gstProgress.h"
#include "fusion/gst/gstIconManager.h"
#include "fusion/gst/gstSourceManager.h"
#include "fusion/gst/gstGridUtils.h"
#include "fusion/gst/gstKmeans.h"
#include "fusion/gst/gstPolygonSimplifier.h"
#include "fusion/gst/gstJobStats.h"
#include "fusion/gst/QuadExporter.h"
#include "fusion/gst/gstSequenceAlg.h"

#include "fusion/gst/JSDisplayBundle.h"
#include "fusion/autoingest/.idl/storage/VectorFuseAssetConfig.h"


bool gstLayer::TryPopulateJSDisplayBundle(JSDisplayBundle *bundle,
                                          QString *error) {
  try {
    // presize the vector, we may only need to populate some, but they all need
    // to exist
    unsigned int numRules = NumFilters();
    bundle->displayJS.resize(numRules);

    for (unsigned int r = 0; r < numRules; ++r) {
      // This filter had no results, don't bother setting up JS on it's behalf
      // it won't have any record to format anyway
      gstFilter *filter = GetFilterById(r);
      if (filter->GetSelectListSize() == 0)
        break;

      {
        // see if we have a JS road label
        const FuseFeatureConfig &fcfg = filter->FeatureConfigs().config;
        if (fcfg.HasRoadLabelJS()) {
          if (!fcfg.style.label.label.isEmpty()) {
            bundle->EnsureContext(this);
            bundle->displayJS[r].roadLabelScript =
              bundle->cx->CompileScript(fcfg.style.label.label,
                                       QString("Road Label (rule %1)")
                                       .arg(r));
          }
        }
      }
      {
        // see if we have a JS label
        const FuseSiteConfig &scfg = filter->Site().config;
        if (scfg.HasLabelJS()) {
          if (!scfg.style.label.label.isEmpty()) {
            bundle->EnsureContext(this);
            bundle->displayJS[r].labelScript =
              bundle->cx->CompileScript(scfg.style.label.label,
                                       QString("Label (rule %1)")
                                       .arg(r));
          }
        }
        // see if we have a JS popup text
        if (scfg.HasPopupTextJS()) {
          if (!scfg.popupText.isEmpty()) {
            bundle->EnsureContext(this);
            bundle->displayJS[r].popupTextScript =
              bundle->cx->CompileScript(scfg.popupText,
                                       QString("Popup Text (rule %1)")
                                       .arg(r));
          }
        }
      }
    }

    return true;
  } catch(const khException &e) {
    *error = e.qwhat();
  } catch(const std::exception &e) {
    *error = e.what();
  } catch(...) {
    *error = "Unknown error";
  }
  return false;
}


// Used to make layer from gstVectorProject
// This happens when loading an entire project into the GUI ProjectManager
gstLayer::gstLayer(gstVectorProject* p, gstSource *src, int srcLayerNum,
                   const LayerConfig& cfg)
    : config(cfg),
      parent_(p),
      the_selector_(NULL),
      simplifier_(),
      culler_() {
  Init();

  if (src != 0)
    the_selector_ = new gstSelector(src, srcLayerNum, cfg,
                                    GetExternalContextScripts());
}

gstLayer::gstLayer(gstVectorProject* p, gstSource *src, const FuseConfig& cfg)
    : config(),
      parent_(p),
      the_selector_(NULL),
      simplifier_(),
      culler_() {
  Init();

  // patch up the layer config with information from the FuseConfig
  config.layerContextScript = cfg.contextScript;

  the_selector_ = new gstSelector(this, src, cfg);
}

void gstLayer::Init() {
  sort_id_ = 0;
  version_ = 1;
  enabled_ = false;
  exporter_ = NULL;
  efficient_lod_ = 0;
  ResetNodeStatistics();
}

void gstLayer::ResetNodeStatistics() {
  node_count_total_ = 0;
  node_count_done_ = 0;
  node_count_covered_ = 0;
  node_count_empty_ = 0;
  node_count_skipped_ = 0;
}

gstLayer::~gstLayer() {
  if (the_selector_)
    the_selector_->unref();
}

void gstLayer::SetConfig(const LayerConfig& newcfg) {
  if (config == newcfg)
    return;
  config = newcfg;

  if (the_selector_) {
    // do not allow drawing while reworking contents
    bool wasEnabled = Enabled();
    SetEnabled(false);

    bool keptQueries = false;
    the_selector_->SetConfig(config, keptQueries,
                             GetExternalContextScripts());

    if (wasEnabled && keptQueries) {
      SetEnabled(true);
    }
  }
}

void gstLayer::SetName(const QString& n) {
  config.defaultLocale.name_ = n;
}

//
// sortid determines the order layers appear
// in the client menu
//
void gstLayer::SetSortId(unsigned int id) {
  if (id != sort_id_) {
    sort_id_ = id;
  }
}

void gstLayer::SetLegend(const QString& l) {
  if (l != config.legend) {
    config.legend = l;
  }
}

void gstLayer::SetVersion(int v) {
  if (version_ != v) {
    version_ = v;
  }
}

gstSource* gstLayer::GetSource() const {
  if (the_selector_ == NULL)
    return NULL;
  return the_selector_->getSource();
}

gstSharedSource gstLayer::GetSharedSource(void) const {
  return theSourceManager->TryGetSharedAssetSource(config.assetRef);
}


int gstLayer::GetSourceLayerNum(void) const {
  if (the_selector_ == NULL)
    return 0;
  return the_selector_->layer();
}

gstHeaderHandle gstLayer::GetSourceAttr() {
  gstHeaderHandle hdr;

  gstSource* src = GetSource();
  if (src) {
    hdr = src->GetAttrDefs(the_selector_->layer());
  } else {
    hdr = gstHeaderImpl::Create();
  }

  return hdr;
}

const char *gstLayer::GetAssetName() const {
  gstSource* src = GetSource();
  return src ? src->name() : NULL;
}


gstFilter* gstLayer::GetFilterById(unsigned int id) const {
  if (id > the_selector_->NumFilters())
    return NULL;
  return the_selector_->GetFilter(id);
}

unsigned int gstLayer::NumFilters(void) const {
  return the_selector_ ? the_selector_->NumFilters() : 0;
}

void gstLayer::ApplyBoxCutter(const gstDrawState& state, GeodeList& list) {
  if (the_selector_) {
    the_selector_->applyBoxCutter(state, list);
  }
}

void gstLayer::ApplyBox(const gstDrawState& state, int mode) {
  if (the_selector_) {
    the_selector_->applyBox(state, mode);
  }
}

bool gstLayer::QueryComplete() const {
  return the_selector_ ? the_selector_->queryComplete() : true;
}

void gstLayer::QueryThreadPrep(gstProgress* progress) {
  query_progress_ = progress;
  query_progress_->SetVal(0);
}

// Called only by the GUI preview code
void gstLayer::QueryThread() {
  bool saved_draw_state = Enabled();
  SetEnabled(false);

  if (the_selector_ != NULL) {
    // allow 20 soft errors
    SoftErrorPolicy soft_errors(20);
    try {
      the_selector_->ThrowingApplyQueries(query_progress_,
                                          soft_errors);
      SetEnabled(saved_draw_state);
      query_progress_->SetSucceeded();
      if (soft_errors.NumSoftErrors() > 0) {
        QString error = kh::tr("Encountered %1 error(s) during query:")
                        .arg(soft_errors.NumSoftErrors());
        for (unsigned int i = 0; i < soft_errors.Errors().size(); ++i) {
          error += "\n";
          error += QString::fromUtf8(soft_errors.Errors()[i].c_str());
        }
        query_progress_->SetWarning(error.toUtf8().constData());
      }
    } catch(const SoftErrorPolicy::TooManyException &e) {
      std::vector<std::string> errors;
      errors.push_back("Too many errors");
      std::copy(e.errors_.begin(), e.errors_.end(), back_inserter(errors));
      query_progress_->SetFatalErrors(errors);
    } catch(const std::exception &e) {
      query_progress_->SetFatalError(e.what());
    } catch(...) {
      query_progress_->SetFatalError("Unknown error during query");
    }
  }
}


void gstLayer::SetEnabled(bool s) {
  if (s && !QueryComplete()) {
    notify(NFY_WARN, "Cannot enable layer until query is completed.");
    return;
  }

  enabled_ = s;

  // need to send this to the format in case a raster
  // layer exists which needs to be toggled.
  gstSource* src = GetSource();
  if (src) {
    src->SetEnabled(s);
  }
}


void gstLayer::DrawFeatures(int* max_count, const gstDrawState& state,
                            bool selected) {
  if (!Enabled())
    return;

  static int maxmsg = 10;
  if (!QueryComplete()) {
    if (maxmsg > 1) {
      notify(NFY_WARN, "Error!  Can't draw until query is completed");
      maxmsg--;
    } else if (maxmsg == 1) {
      notify(NFY_WARN, "Error!  Can't draw until query is completed");
      notify(NFY_WARN, "  (suppressing further messages)");
      maxmsg--;
    }
    return;
  }

  if (the_selector_ != NULL) {
    if (selected) {
      the_selector_->drawSelectedFeatures(state);
    } else {
      the_selector_->drawFeatures(max_count, state);
    }
  }
}


void gstLayer::GetSiteLabels(const gstDrawState& state,
                             std::vector<gstSiteSet>* site_sets) {
  site_sets->resize(the_selector_->NumFilters());
  int current_site = 0;

  if (the_selector_ == NULL)
    return;

  JSDisplayBundle jsbundle;
  QString error;
  if (!TryPopulateJSDisplayBundle(&jsbundle, &error)) {
    notify(NFY_WARN, "Error Prepping JS for labels: %s",
           (const char *)error.utf8());
    site_sets->resize(0);
    return;
  }


  for (unsigned int f = 0; f < the_selector_->NumFilters(); ++f) {
    if (the_selector_->GetFilter(f)->Site().config.enabled) {
      if (!the_selector_->PrepareSiteLabels(
          state.cullbox, static_cast<std::int32_t>(f), site_sets->at(current_site),
          jsbundle, state.IsMercatorPreview())) {
        site_sets->resize(0);
        return;
      }
      if (site_sets->at(current_site).vlist.size() > 0) {
        ++current_site;
      }
    }
  }

  site_sets->resize(current_site);
}

////////////////////////////////////////////////////////////////////////////////

gstLayer::BuildSetMgr::BuildSetMgr(gstSelector* sel)
    : selector_(sel) {
  notify(NFY_NOTICE, "Computing extents of all filters for this layer");

  //
  // cycle through all filters, adding a build set for each
  // enabled feature and site
  //
  for (unsigned int f = 0; f < selector_->NumFilters(); ++f)
    AddFilter(f);
}

void gstLayer::BuildSetMgr::AddFilter(int filter_id) {
  gstFilter* filter = selector_->GetFilter(filter_id);

  if (filter->GetSelectListSize() == 0)
    return;

  const gstBBox& bbox = filter->GetBoundingBox();

  const FuseFeatureConfig &fuse_config = filter->FeatureConfigs().config;

  if (fuse_config.featureType != VectorDefs::PointZ) {
    // polylines, roads, polygons, points
    build_area_[fuse_config.minLevel].Grow(bbox);

    notify(NFY_NOTICE, "  Filter %d has %d features on level %d",
           filter_id, filter->GetSelectListSize(), fuse_config.minLevel);

    gstPrimType type = gstUnknown;

    if (fuse_config.featureType == VectorDefs::LineZ) {
      if (fuse_config.drawAsRoads) {
        type = gstStreet;
        road_levels_.set(fuse_config.minLevel);
      } else {
        type = gstPolyLine;
      }
    } else if (fuse_config.featureType == VectorDefs::PolygonZ) {
      type = gstPolygon;
    }

    box_list_[fuse_config.minLevel].push_back(
        BuildSet(filter_id, filter->GetGeoIndex(),
                 type, fuse_config.maxLevel, fuse_config.maxBuildLevel));
  }

  const gstSite& site = filter->Site();
  if (site.config.enabled) {
    build_area_[site.config.minLevel].Grow(bbox);
    // If point plotting build upto max "Draw Label Visibility"
    // else use feature's fuse_config.maxBuildLevel to limit building.
    const unsigned int max_site_build_level =
        (fuse_config.featureType == VectorDefs::PointZ ?
         site.config.maxLevel : fuse_config.maxBuildLevel);

    box_list_[site.config.minLevel].push_back
      (BuildSet(filter_id, filter->GetGeoIndex(),
                gstPoint, site.config.maxLevel, max_site_build_level));

    notify(NFY_NOTICE, "  Filter %d has %d sites on level %d",
           filter_id, filter->GetSelectListSize(), site.config.minLevel);
    khLevelCoverage cov = filter->GetGeoIndex()->GetCoverage();
    notify(NFY_NOTICE, "         %u x %u = %Zu cells (or %u per cell)",
           cov.extents.numRows(), cov.extents.numCols(),
           cov.NumTiles(),
           static_cast<std::uint32_t>(filter->GetSelectListSize()/cov.NumTiles()));
  }
}

void gstLayer::BuildSetMgr::Update(unsigned int lev, bool need_lod) {
  unsigned int max_end = 0;
  unsigned int min_end = MaxClientLevel;
  unsigned int max_build_level = 0;

  // examine all boxes to see what might need to get pushed down
  for (unsigned int s = 0; s < box_list_[lev].size(); ++s) {
    unsigned int end = box_list_[lev][s].endlevel;
    max_end = std::max(end, max_end);
    min_end = std::min(end, min_end);
    unsigned int max_build_level_s = box_list_[lev][s].max_build_level;
    max_build_level = std::max(max_build_level_s, max_build_level);
  }

  notify(NFY_NOTICE,
         "Level %d: Filter default fadeout is level %d, max end is %d, "
         "max build level is %d.",
         lev, min_end, max_end, max_build_level);

  unsigned int fadeout = min_end;

  if (getenv("KH_MOBILE")) {
    fadeout = lev + 1;
    notify(NFY_NOTICE, "Mobile packets must be re-inserted every level.");
  }

  //
  // Adjust for roads
  //
  // Roads must be faded out every few levels and re-inserted since
  // they are drawn with extruded geometry from the road centerline and
  // the geometry is not resized so it will continue to appear larger and
  // larger as the view gets closer
  if (road_levels_.get(lev)) {
    if (fadeout != lev) {
      unsigned int pushroads = 2;
      if (road_levels_.get(lev + 1)) {
        pushroads = 1;
      } else if (road_levels_.get(lev + 2)) {
        pushroads = 2;
      } else if (road_levels_.get(lev + 3)) {
        pushroads = 3;
      }

      // Do the roads fadeout before our default fadeout? If so set the fadeout.
      if ((lev + pushroads) < fadeout)
        fadeout = lev + pushroads;

      notify(NFY_NOTICE, "Roads existence forces re-insertion at level %d.",
             fadeout);
    } else if (fadeout == lev) {
      notify(NFY_NOTICE, "Road fadeout is at the max level %d.",
              fadeout);
    }
  }

  //
  // Adjust for lods
  //
  // the need_lod flag will be true if any quad on this level needed
  // to be simplified to fit the size constraints.  if this happened,
  // we want to re-insert the geometry two levels lower hoping to get
  // higher quality.
  if ((fadeout > (lev + 3)) && need_lod) {
    fadeout = lev + 2;
    notify(NFY_NOTICE, "Simplification forces re-insertion at level %d.",
           fadeout);
  }

  //
  // Adjust for any really low-res vectors
  //
  // very simple vectors placed at low levels might not require lods
  // so they will never need to be re-inserted.  this can be a problem
  // when terrain is turned on because only vertexes are raised above
  // the terrain.  if the vector is very coarse, there could be a huge
  // distance between vertexes and even the curvature of the earth will
  // eventually be enough to cause the vector to cut into the surface
  //
  // but we don't want to do this if there are only points, so scan
  // all build sets first
  if (lev < 9 && fadeout > (lev + 4)) {
    unsigned int ftmp = lev + 2;
    for (unsigned int s = 0; s < box_list_[lev].size(); ++s) {
      const BuildSet& bs = box_list_[lev][s];
      if ((bs.endlevel > ftmp) && (bs.type == gstStreet ||
                                   bs.type == gstPolyLine)) {
        fadeout = ftmp;
        notify(NFY_NOTICE,
               "Low resolution vectors force re-insertion at level %d.",
               fadeout);
        break;
      }
    }
  }

  // Our last fadeout level will be at the max_build level.
  if (fadeout > max_build_level) {
    fadeout = max_end;  // Force it to max visibility level since we're not
                        // building further levels.
  }
  notify(NFY_NOTICE, "Level %d fading out to %d.", lev, fadeout + 1);
  fadeout_levels_.set(lev, fadeout + 1);

  // Nothing to push down after the fadeout level hits the max_end.
  // Note: fadeout begins after the "fadeout" level so it is technically
  // visible at that level.
  if (fadeout >= max_end) {
    notify(NFY_NOTICE,
           "Final fadeout is greater than maximum level. "
           "No need to push additional LODs.");
    return;
  }

  if (fadeout > lev) {
    // step through every buildset, pushing down if required
    for (unsigned int s = 0; s < box_list_[lev].size(); ++s) {
      const BuildSet &bs = box_list_[lev][s];
      if (bs.endlevel > fadeout) {  // No need to push additional LODs past the
                                    // end level for any build set.
        box_list_[fadeout].push_back(bs);
        build_area_[fadeout].Grow(build_area_[lev]);
        // if roads were pushed, update the roadLevel flags
        if (bs.type == gstStreet)
          road_levels_.set(fadeout);
      }
    }
  }
}


//
// save out all lod switch points in simple text file
//
bool gstLayer::BuildSetMgr::DumpLODTable(const QString& path) {
  QFile file(path);
  if (!file.open(IO_WriteOnly)) {
    notify(NFY_WARN, "Unable to save LOD Table: %s", file.name().latin1());
    return false;
  }

  QTextStream stream(&file);
  for (unsigned int lev = 0; lev < MAX_CLIENT_LEVEL; ++lev)
    stream << fadeout_levels_[lev] << endl;
  file.close();

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool gstLayer::ExportStreamPackets(geFilePool &file_pool,
                                   const std::string& pub_dir) {
  // empty layers do nothing!
  if (the_selector_ == NULL)
    return true;
  notify(NFY_DEBUG, "gstLayer::ExportStreamPackets()");

  // make sure version number is current with the project
  SetVersion(parent_->buildVersion());

  //
  // query must have successfully completed
  // if not, we should skip this layer
  //
  if (!QueryComplete()) {
    notify(NFY_WARN, "Aborting due to unsuccessful query");
    return false;
  }

  JSDisplayBundle jsbundle;
  QString error;
  if (!TryPopulateJSDisplayBundle(&jsbundle, &error)) {
    notify(NFY_WARN, "Error Prepping JS for export: %s",
           (const char *)error.utf8());
    return false;
  }

  // Calculate Efficient LOD.
  // At Efficient Resolution Level the average feature of layer has
  // diameter 1/8 of tile size.
  efficient_lod_ = EfficientLOD(the_selector_->AverageSrcFeatureDiameter(),
                                ClientVectorTilespace,
                                ClientVectorTilespace.pixelsAtLevel0 / 8.0);

  try {
    exporter_ = NewPacketFileExporter(file_pool, pub_dir);
    khDeleteGuard<gstExporter> exporter_guard(TransferOwnership(exporter_));
    if (!exporter_->Open()) {
      notify(NFY_WARN, "Unable to open packet file %s", pub_dir.c_str());
      return false;
    } else {
      notify(NFY_INFO, "Writing packet file %s", pub_dir.c_str());
    }

    BuildSetMgr build_set_manager(the_selector_);

    QTime level_timer;

    for (unsigned int lev = 0; lev <= MAX_CLIENT_LEVEL; ++lev) {
      if (!build_set_manager.IsLevelValid(lev))
        continue;

      ResetNodeStatistics();

      level_timer.restart();

      gstBBox box = build_set_manager.GetBBox(lev);

      //
      // estimate how many quadnodes need to be processed based on bounding box
      // this estimate will progressively get more accurate as the box is
      // partitioned
      //
      std::uint32_t min_row, max_row, min_col, max_col;
      // TODO: use BoxLatLonToRowCol(), then it can be calculated
      // more precise, otherwise we get +1 col and +1 row in case of geode
      // bounding box coincide with quad boundary.
      LatLonToRowCol(lev, box.s, box.w, min_row, min_col);
      LatLonToRowCol(lev, box.n, box.e, max_row, max_col);

      int rows = max_row - min_row + 1;
      int cols = max_col - min_col + 1;
      node_count_total_ = static_cast<std::int64_t>(rows) * static_cast<std::int64_t>(cols);

      notify(
          NFY_NOTICE,
          "#################################################################");
      notify(NFY_NOTICE,
             "Begin export of level %d, rows:%d, cols:%d, total tiles:%Zu",
             lev, rows, cols,
             node_count_total_);
#if 0
      notify(NFY_NOTICE, " rows: %u->%u  cols: %u->%u",
             min_row, max_row + 1, min_col, max_col + 1);
#endif
      notify(
          NFY_NOTICE,
          "#################################################################");

      khExtents<std::uint32_t> tiles(RowColOrder, min_row, max_row + 1, min_col,
                              max_col + 1);

      bool need_lod = false;  // This will be set to true by ExportQuad
                              // if it deems that LODs are needed to render
                              // this layer accurately (generally more than
                              // 1 simplification pass needed at the 1st level).
      if (!BuildRegion(tiles, lev, build_set_manager.GetBoxList(lev),
                       &need_lod, jsbundle)) {
        // any failure is fatal, terminate build now
        throw khException("Unable to build region");
      }

      // any decimation triggers an LOD
      build_set_manager.Update(lev, need_lod);

      notify(NFY_NOTICE, "Elapsed time = %.3lf seconds",
             static_cast<float>(level_timer.elapsed()) / 1000.0);
      notify(NFY_NOTICE,
             "Tiles completed:%Zu, covered:%Zu, empty:%Zu, skipped:%Zu",
             node_count_done_,
             node_count_covered_,
             node_count_empty_,
             node_count_skipped_);
    }
    exporter_->Close();
    return build_set_manager.DumpLODTable(QDir(pub_dir.c_str()).filePath("lodtable"));
  } catch(const std::exception &e) {
    notify(NFY_WARN, "Error during export: %s", e.what());
  } catch(...) {
    notify(NFY_WARN, "Unknown error during export");
  }
  return false;
}


#ifdef JOBSTATS_ENABLED
enum { JOBSTATS_BUILD, JOBSTATS_EXPORT, JOBSTATS_PREPARE,
       JOBSTATS_MAINLOOP, JOBSTATS_SIMPLIFY, JOBSTATS_SIMLOOP,
       JOBSTATS_BUILDPACKET, JOBSTATS_SAVE };

static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_BUILD,       "Build Region     "},
  {JOBSTATS_EXPORT,      "Export Quad      "},
  {JOBSTATS_PREPARE,     "+ Prepare        "},
  {JOBSTATS_MAINLOOP,    "+ Main Loop      "},
  {JOBSTATS_SIMPLIFY,    "+ + Simplify     "},
  {JOBSTATS_SIMLOOP,     "+ + + Sim loop   "},
  {JOBSTATS_BUILDPACKET, "+ + Build Packet "},
  {JOBSTATS_SAVE,        "+ + Save         "}
};
gstJobStats* build_stats = new gstJobStats("Export Layer", JobNames, 8);
#endif

bool gstLayer::BuildRegion(const khExtents<std::uint32_t>& tiles, int level,
                           std::vector<BuildSet>& buildset_list,
                           bool* need_lod,
                           JSDisplayBundle &jsbundle) {
  JOBSTATS_BEGIN(build_stats, JOBSTATS_BUILD);
#if 0
  notify(NFY_VERBOSE, "BuildRegion(): level: %d", level);

  for (size_t i = 0; i < buildset_list.size(); ++i) {
    notify(NFY_VERBOSE, "buildSets[%lu] size: %d",
           i, buildset_list[i].geoIndex->GetCount());
  }
#endif

  // determine the coverage we need to traverse at each level (from 0 to level)
  khInsetCoverage insetCov(ClientVectorTilespace,
                           khLevelCoverage(level, tiles),
                           0 /* beginLevel Coverage */,
                           level+1 /* endLevelCoverage */);

  // add block to control lifetime of progress meter - and thereby the
  // final report
  {
    khLevelCoverage targetCov = insetCov.levelCoverage(level);

    khProgressMeter progress(targetCov.NumTiles());

    // Holds original BuildSets GeoIndexes for current level.
    std::vector<gstGeoIndexHandle> buildset_indexes;
    buildset_indexes.reserve(buildset_list.size());

    for (size_t i = 0; i < buildset_list.size(); ++i) {
      buildset_indexes.push_back(buildset_list[i].geoIndex);
    }

    // build the full res exporter at our target level
    // this is the one that eventually calls gstLayer::ExportQuad (below)
    khDeletingVector<QuadExporterBase> exporters;
    exporters.reserve(level);
    exporters.push_back(new FullResQuadExporter(this, &progress,
                                                buildset_list,
                                                &buildset_indexes,
                                                need_lod,
                                                targetCov,
                                                jsbundle));

    // build all the minified levels up to level 0
    // use int (instead of unsigned int) to avoid wrap at 0
    for (int lev = level - 1; lev >= 0; --lev) {
      exporters.push_back(new MinifiedQuadExporter
                          (exporters.back(),
                           insetCov.levelCoverage(lev),
                           targetCov));
    }

    // now just tell the top most exporter (level 0) to export 0,0
    // everything else will fall out from there
    if (!exporters.back()->ExportQuad(0, 0)) {
      return false;
    }
  }

  JOBSTATS_END(build_stats, JOBSTATS_BUILD);

  JOBSTATS_DUMPALL();
  JOBSTATS_RESETALL();

  the_selector_->DumpBuildStats();

  return true;
}

// Maximum number of loops to attempt to get a packet small enough to fit into
// the max packet size.  Each time through the loop doubles the allowable pixel
// error in the geode simplification algorithm.
#define MAX_LOOP_COUNT 20

#undef DUMP_FEATURE_QUAD

bool gstLayer::ExportQuad(const gstQuadAddress& quad_address,
                          FullResQuadExporter &exporter) {
  JOBSTATS_SCOPED(build_stats, JOBSTATS_EXPORT);

  notify(NFY_DEBUG, "Exporting quad: level=%d row=%u col=%u list=%Zu",
         quad_address.level(), quad_address.row(), quad_address.col(),
         exporter.buildSets.size());

  //
  // allocate feature sets and site sets for the maximum possible
  // size so that we can then pass a handle to the already allocated
  // set to gstSelector.  this eliminiates copying of a std::vector
  //
  // reduce to the correct size after all build sets have been examined.
  //

  JOBSTATS_BEGIN(build_stats, JOBSTATS_PREPARE);

  int curr_feature_set = 0;
  int curr_site_set = 0;

  // these should already be empty, but it's cheap to do it again
  // and if for some reason they weren't empty, we'd have extraneous
  // features to intersect
  exporter.feature_sets.resize(0);
  exporter.site_sets.resize(0);

  // separate buildset into features and sites
  //
  // expand feature id's into geometry/attributes with the prepare*() calls
  //
  // ideally the buildset type should be VectorDefs::FeatureType
  // but since it needs to distinguish between streets and polylines
  // we have to keep the type as gstPrimType. This is very misleading though.
  for (unsigned int build_set_id = 0; build_set_id < exporter.numSets; ++build_set_id) {
    if (exporter.UsedSets()[build_set_id]) {
      const BuildSet& b = exporter.buildSets[build_set_id];
      bool build_set_has_geometry = false;

      const gstGeoIndexImpl::FeatureBucket *bucket = 0;
      gstGeoIndexImpl::FeatureBucket *tmpBucket = 0;
      if (quad_address.level() == b.geoIndex->GetCoverage().level) {
        // we are exporting a single cell from the index
        // Just get a pointer to the bucket
        assert(b.geoIndex->GetCoverage().extents.ContainsRowCol
               (quad_address.row(), quad_address.col()));

        bucket = b.geoIndex->GetBucket(quad_address.row(), quad_address.col());
      } else if (quad_address.level() < b.geoIndex->GetCoverage().level) {
        // this index is deeper than we are exporting, we need to merge
        // several FeatureBuckets before proceeding
        // we are exporting a single cell from the index
        // NOTE: This should be rare
        tmpBucket = new gstGeoIndexImpl::FeatureBucket();

        khLevelCoverage exportCov =
            khTileAddr(quad_address.level(),
                       quad_address.row(), quad_address.col()).
            MagnifiedToLevel(b.geoIndex->GetCoverage().level);

        khExtents<std::uint32_t> toCheck =
            khExtents<std::uint32_t>::Intersection(exportCov.extents,
                                            b.geoIndex->GetCoverage().extents);

        b.geoIndex->PopulateBucket(toCheck, tmpBucket);
        bucket = tmpBucket;
      } else {
        notify(NFY_FATAL,
               "Internal Error: ExportQuad called on un-split index");
      }

      switch (b.type) {
        case gstUnknown:
        case gstPoint25D:
        case gstPolyLine25D:
        case gstStreet25D:
        case gstPolygon25D:
        case gstPolygon3D:
        case gstMultiPolygon:
        case gstMultiPolygon25D:
        case gstMultiPolygon3D:
          notify(NFY_FATAL, "Improper build set type %d.", b.type);
          break;

        case gstPoint:
          exporter.site_sets.resize(curr_site_set + 1);
          if (the_selector_->PrepareSites(quad_address, b.filterId,
                                          *bucket,
                                          b.geoIndex->GetFeatureList(),
                                          exporter.site_sets[curr_site_set],
                                          exporter.JSBundle())) {
            ++curr_site_set;
          }
          break;

        case gstPolyLine:
        case gstStreet:
        case gstPolygon:
          exporter.feature_sets.resize(curr_feature_set + 1);

#ifdef DUMP_FEATURE_QUAD
          fprintf(stderr, "QUAD:%u,%u,%u:%d:",
                  quad_address.level(),
                  quad_address.row(),
                  quad_address.col(), b.filterId);
          for (unsigned int i = 0; i < bucket->size(); ++i) {
            fprintf(stderr, "%u,",
                    b.geoIndex->GetFeatureList()[(*bucket)[i]].feature_id);
          }
          fprintf(stderr, "\n");
#endif
          if (the_selector_->PrepareFeatures(
                  quad_address, b.filterId,
                  (*(exporter.buildset_indexes_))[build_set_id],
                  *bucket,
                  b.geoIndex->GetFeatureList(),
                  &exporter.feature_sets[curr_feature_set],
                  exporter.JSBundle())) {
#ifdef DUMP_FEATURE_QUAD
            fprintf(stderr, "QUAD:%u,%u,%u:PREPARED\n",
                    quad_address.level(),
                    quad_address.row(),
                    quad_address.col());
#endif
            ++curr_feature_set;
            build_set_has_geometry = true;
          }
          break;
      }

      // Note: Do not update presence mask for gstPoint build sets
      // (no quad-tree partitioning optimization for gstPoint build sets).
      // The reason: the labels that are introduced with vector layer
      // belong to the same filter (DisplayRule) and use the same geoIndex
      // (presence mask) as the geometry of the filter.
      if (b.type != gstPoint && !build_set_has_geometry &&
          quad_address.level() <= kMaxLevelForBuildPresenceMask) {
#if 0
        notify(NFY_VERBOSE, "build_set: %u - (l:%u, r:%u, c:%u) is empty",
               build_set_id,
               quad_address.level(), quad_address.row(), quad_address.col());
#endif
        // Update presence mask based on result of clipping for current
        // build set.
        (*(exporter.buildset_indexes_))[build_set_id]->SetPresence(
            khTileAddr(quad_address.level(),
                       quad_address.row(),
                       quad_address.col()),
            false);
      }

      if (tmpBucket) {
        delete tmpBucket;
      }
    }
  }

  // remove empty objects at the end
  exporter.feature_sets.resize(curr_feature_set);
  exporter.site_sets.resize(curr_site_set);

  JOBSTATS_END(build_stats, JOBSTATS_PREPARE);

  // nothing to do afterall
  if (curr_feature_set == 0 && curr_site_set == 0) {
    ++node_count_empty_;
    return true;
  }

  int paksize = -1;
  bool small_paksize_to_save = false;

  // Track the simplification iterations and terminate when the maximum
  // number of iterations has been reached.
  int loopcount = 0;

  JOBSTATS_BEGIN(build_stats, JOBSTATS_MAINLOOP);

  do {  // Try from 0 to MAX_LOOP_COUNT, i.e MAX_LOOP_COUNT+1 times
    JOBSTATS_BEGIN(build_stats, JOBSTATS_SIMPLIFY);
    if (exporter.feature_sets.size()) {
      double max_error =
          SimplifyFeatureSet(&(exporter.feature_sets), quad_address.level(),
                             loopcount);
      // If any simplification occurs (other than removing redundant points),
      // then this indicates LOD is needed.
      if (max_error > 0.0)
        *exporter.needLOD = true;
    }
    if (exporter.site_sets.size()) {
      bool is_simplified =
          SimplifySiteSet(&(exporter.site_sets), quad_address.level(),
                          loopcount);
      // If any simplification occurs (other than removing redundant points),
      // then this indicates LOD is needed.
      // If we simplified the number of sites, mark as in need of LOD.
      if (is_simplified)
        *exporter.needLOD = true;
    }
    JOBSTATS_END(build_stats, JOBSTATS_SIMPLIFY);

    //
    // this is where the actual packet is built
    //
    JOBSTATS_BEGIN(build_stats, JOBSTATS_BUILDPACKET);
    paksize = exporter_->BuildPacket(quad_address,
                                     exporter.feature_sets,
                                     exporter.site_sets);
    JOBSTATS_END(build_stats, JOBSTATS_BUILDPACKET);

    //
    // If any packet was too big, try to build
    // by decimating until we either fit, or reach
    // our max error threshhold;
    // Else small_paksize_to_save == true
    //
    if (paksize != -1) {
      if (loopcount == MAX_LOOP_COUNT) {
        // we let the packet slip through if it's less than the MAX
        small_paksize_to_save = exporter_->MaxPacketSize(paksize);
      } else {
        small_paksize_to_save = exporter_->TargetPacketSize(paksize);
      }
    }
    ++loopcount;
  } while (!small_paksize_to_save && loopcount <= MAX_LOOP_COUNT);

  bool success = false;
  if (small_paksize_to_save) {
    JOBSTATS_BEGIN(build_stats, JOBSTATS_SAVE);
    success = exporter_->Save();
    JOBSTATS_END(build_stats, JOBSTATS_SAVE);
  } else {
    notify(NFY_WARN,
           "Can't simplify packet any more (l:%d r:%u c:%u size:%d)",
           quad_address.level(), quad_address.row(), quad_address.col(),
           paksize);
  }

  JOBSTATS_END(build_stats, JOBSTATS_MAINLOOP);

  // cleanup all geodes that were generated here.
  exporter.feature_sets.resize(0);
  exporter.site_sets.resize(0);

  ++node_count_done_;
  return success;
}

//////////////////////////////////////////////
// simplification of 2.5D polygons
//
void gstLayer::simplify25DPolygons(GeodeList *glist,
                                   int loopcount,
                                   int *geodes_removed,
                                   int *vertices_removed) {
  // Eliminate x% of the geodes where x depends
  // on loopcount. The smallest geodes will be picked first.

  GeodeList::size_type const glistSize = glist->size();
  int eliminateCount  = static_cast<int>(0.7 * (static_cast<double>(loopcount) /
                                                MAX_LOOP_COUNT) * glistSize);
  notify(NFY_DEBUG, "  eliminatecount: %d, loopcount: %d",
         eliminateCount, loopcount);

  // In the following case no more simplification can be done. Returning from
  // here makes it easier for rest of algorithm to assume
  // glistSize != 0 && eliminateCount != 0
  if (glistSize == 0 || eliminateCount == 0) {
    return;
  }
  std::multimap<double, size_t> mmArea;

  for (GeodeList::size_type i = 0; i < glistSize; ++i) {
    const gstGeodeHandle& geodeh = (*glist)[i];
    if (geodeh->PrimType() != gstPolygon25D)
      continue;

    const gstGeode *geode = static_cast<const gstGeode*>(&(*geodeh));

    // Note: it is switched off because these polygons can be
    // eliminated only at low-resolution levels. So it doesn't affect on display
    // at high-resolution levels but improves display at low-resolution levels.
#if 0
    // don't eliminate the polygons that are clipped
    if (geode->IsClipped())
      continue;
#endif

    mmArea.insert(std::pair<double, size_t>(geode->Volume(), i));
  }
  if (mmArea.empty()) {
    return;
  }

  std::multimap<double, size_t>::iterator immArea = mmArea.begin();
  std::vector<bool> delete_flag(glistSize, false);

  while (eliminateCount > 0 && immArea != mmArea.end()) {
    delete_flag[immArea->second] = true;
    const gstGeodeHandle& geodeh = (*glist)[immArea->second];
    *vertices_removed += geodeh->TotalVertexCount();

    --eliminateCount;
    ++immArea;
  }

  ShrinkToRemainingUsingSwap<GeodeList, GeodeList::iterator, false>(
      delete_flag, glist);
  const size_t deleteCount = glistSize - glist->size();
  *geodes_removed += static_cast<int>(deleteCount);

  if (deleteCount > 0) {
    notify(NFY_NOTICE,
           "  Deleted %lu features attempting to reduce packet size.",
           deleteCount);
  }
}

//////////////////////////////////////////////
// simplification of 3D polygons
//
void gstLayer::simplify3DPolygons(GeodeList *glist,
                                  int loopcount,
                                  int *geodes_removed,
                                  int *vertices_removed) {
  if (glist->empty()) {
    return;
  }

  // numCells should be based on pixel error
  // right now im just using 100/loopcount
  int numCells = 100/loopcount;

  const GeodeList::size_type glist_size = glist->size();

  std::vector<fusion_gst::psPolygon> polygons;
  polygons.reserve(glist_size);

  for (unsigned int i = 0; i < glist_size; ++i) {
    fusion_gst::psPolygon polygon;
    const gstGeodeHandle &geodeh = (*glist)[i];
    if (geodeh->PrimType() == gstMultiPolygon ||
        geodeh->PrimType() == gstMultiPolygon25D ||
        geodeh->PrimType() == gstMultiPolygon3D) {
      notify(NFY_FATAL, "%s: Improper feature type %d.",
             __func__, geodeh->PrimType());
    } else {
      const gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
      *vertices_removed += geode->VertexCount(0);
      for (unsigned int j = 0; j < geode->VertexCount(0); ++j) {
        polygon.AddVertex(geode->GetVertex(0, j),
                          geode->IsInternalVertex(j));
      }
    }
    polygons.push_back(polygon);
  }

  fusion_gst::PolygonSimplifier simplifier;
  simplifier.Decimate(polygons, numCells);

  // re-create the glist with simplified polygons
  glist->clear();
  for (unsigned int i = 0; i < polygons.size(); ++i) {
    const fusion_gst::psPolygon& polygon = polygons[i];

    gstGeodeHandle geodeh = gstGeodeImpl::Create(gstPolygon3D);
    gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

    for (unsigned int j = 0; j < polygon.v.size(); ++j) {
      geode->AddVertex(polygon.v[j]);
      // TODO: edge flag is not propagated?!
      // geode->AddEdgeFlag(polygon.EdgeFlag(j));
      geode->AddEdgeFlag(kNormalEdge);
    }

    glist->push_back(geodeh);
  }

  const size_t delete_count = glist_size - glist->size();
  if (delete_count > 0) {
    notify(NFY_NOTICE,
           "  Deleted %lu features attempting to reduce packet size.",
           delete_count);
  }

  *geodes_removed += static_cast<int>(delete_count);

  // TODO: calculate vertices_removed in PolygonSimplifier?!
  for (unsigned int i = 0; i < glist->size(); ++i) {
    const gstGeodeHandle &geodeh = (*glist)[i];
    const gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
    *vertices_removed -= geode->VertexCount(0);
  }
}


//
// Simplify each geode in set with given sfactor until
// vertex count is no longer getting reduced.
//
double gstLayer::SimplifyFeatureSet(std::vector<gstFeatureSet>* featureSets,
                                    int level, int loopcount) {
  int geodes_removed = 0;
  int vertices_removed = 0;
  double max_simplification_error = 0.0;  // The maximum error of the simplified
  // feature set.
  double overall_max_error = 0;

  for (std::vector<gstFeatureSet>::iterator feature_set = featureSets->begin();
       feature_set != featureSets->end(); ++feature_set) {
    if (feature_set->glist.size() == 0)
      continue;

    if (feature_set->glist[0]->PrimType() == gstPolygon25D) {
      // this one is outside the for loop because it works by eliminating
      // entire geodes rather than just eliminating vertices of individual
      // geodes.
      // Note: usually 2.5D polygons are some buildings so we do
      // not simplify polygon cycles.

      // Cull 2.5D polygons based on feature diameter. It is always applied for
      // all quads of levels that are less than Efficient LOD. It culls features
      // that are too small to affect the display with the same threshold for
      // all of quads of level and so it ensures uniform distribution of
      // features throughout the level.
      // TODO: Actually we should apply it for all levels but in
      // this case it can be needed to build more levels to display small
      // features with required resolution.
      if (level < efficient_lod_ && 0 == loopcount) {
        // Set FeatureCuller. Cull the features with diameter less then
        // max_error-pixels at the corresponding level of detail.
        double max_error = feature_set->feature_configs->config.maxError;
        culler_.set_pixel_error(max_error);
        culler_.ComputeThreshold(level);

        // copy all features that meet the subpixel sizing test.
        GeodeList temp_glist;
        RecordList temp_rlist;
        GeodeListIterator g = feature_set->glist.begin();
        RecordListIterator r = feature_set->rlist.begin();

        int geodes_culled = 0;
        for (; g != feature_set->glist.end(); ++g, ++r) {
          if (!(*g)->IsDegenerate() && !culler_.IsSubpixelFeature(*g)) {
            temp_glist.push_back(*g);
            temp_rlist.push_back(*r);
          } else {
            ++geodes_culled;
            vertices_removed += (*g)->TotalVertexCount();
          }
        }

        geodes_removed += geodes_culled;
        if (geodes_culled) {
          notify(NFY_DEBUG, "  Deleted %d subpixel features.", geodes_culled);
        }

        feature_set->glist.swap(temp_glist);
        feature_set->rlist.swap(temp_rlist);
      }

      // Cull 2.5D polygons based on volume to fit packet size.
      if (loopcount > 0) {
        simplify25DPolygons(&(feature_set->glist), loopcount,
                            &geodes_removed, &vertices_removed);
      }

      // For Polygon25D we just check if at least one geode
      // is removed then set max_simplification_error to 1.0. This means that
      // a subsequent LOD is needed.
      if (geodes_removed) {
        max_simplification_error = 1.0;
      }
      // TODO: Estimate overall_max_error - some metric is needed.
    } else if (feature_set->glist[0]->PrimType() == gstPolygon3D) {
      // simplify 3D polygons
      // this one is outside the for loop because it works by eliminating
      // entire geodes rather than just eliminating vertices of individual
      // geodes.
      if (loopcount > 0) {
        simplify3DPolygons(&(feature_set->glist), loopcount,
                           &geodes_removed, &vertices_removed);
      }

      // For Polygon3D we just check if at least one geode
      // is removed then set max_simplification_error to 1.0. This means that
      // a subsequent LOD is needed.
      if (geodes_removed) {
        max_simplification_error = 1.0;
      }
      // TODO: Estimate overall_max_error - some metric is needed.
    } else {
      // TODO: Do remove IsSubpixelFeature()-functionality from
      // gstSimplifier. And Do make refactoring of SimplifyFeatureSet() - use
      // FeatureCuller for all geode-types except (?)3D-polygons.

      // Simplify other geode types
      double max_error = feature_set->feature_configs->config.maxError;
      if (max_error > overall_max_error) {
        overall_max_error = max_error;
      }
      simplifier_.set_pixel_error(max_error);
      simplifier_.ComputeThreshold(level, loopcount);

      JOBSTATS_BEGIN(build_stats, JOBSTATS_SIMLOOP);

      // copy all features that meet the subpixel sizing test
      GeodeList temp_glist;
      RecordList temp_rlist;
      GeodeListIterator g = feature_set->glist.begin();
      RecordListIterator r = feature_set->rlist.begin();

      for (; g != feature_set->glist.end(); ++g, ++r) {
        if ((*g)->PrimType() == gstMultiPolygon ||
            (*g)->PrimType() == gstMultiPolygon25D ||
            (*g)->PrimType() == gstMultiPolygon3D) {
          notify(NFY_FATAL, "%s: Improper feature type %d.",
                 __func__, (*g)->PrimType());
        }

        if (!(*g)->IsDegenerate() && !simplifier_.IsSubpixelFeature(*g)) {
          temp_glist.push_back(*g);
          temp_rlist.push_back(*r);
        } else {
          ++geodes_removed;
          vertices_removed += (*g)->TotalVertexCount();
        }
      }

      if (geodes_removed) {
        notify(NFY_DEBUG, "  Deleted %d subpixel features.", geodes_removed);
      }

      // reduce vertexes of remaining features
      for (GeodeListIterator geodeh = temp_glist.begin();
           geodeh != temp_glist.end(); ++geodeh) {
        std::vector<int> vertexes_to_keep;
        double simplification_error =
            simplifier_.Simplify(*geodeh, &vertexes_to_keep);
        max_simplification_error =
            std::max(simplification_error, max_simplification_error);

        gstGeode *geode = static_cast<gstGeode*>(&(**geodeh));
        vertices_removed += geode->Simplify(vertexes_to_keep);
      }

      feature_set->glist.swap(temp_glist);
      feature_set->rlist.swap(temp_rlist);

      JOBSTATS_END(build_stats, JOBSTATS_SIMLOOP);
    }
  }

  the_selector_->UpdateSimplifyStats(geodes_removed, vertices_removed,
                                     pow(2.0, loopcount) * overall_max_error);

  // If at least one geode is removed then set max_simplification_error to 1.0.
  // This means that a subsequent LOD is needed.
  if (geodes_removed) {
    max_simplification_error = 1.0;
  }

  return max_simplification_error;
}

bool gstLayer::SimplifySiteSet(std::vector<gstSiteSet>* siteSets,
                               int level, int loopcount) {
  // need this to report build stats
  unsigned int beginSiteCount = 0;
  unsigned int endSiteCount = 0;

  // each site in this set needs to be decimated
  for (unsigned int s = 0; s < siteSets->size(); ++s) {
    gstSiteSet& set = (*siteSets)[s];
    beginSiteCount += set.vlist.size();

    // simplify based on specific number of sites per quad
    std::vector<int> keep;
    if (set.site->config.decimationMode == SiteConfig::RepSubset) {
      unsigned int maxCount = uint(set.vlist.size() *set.site->config.decimationRatio);
      if (maxCount < set.site->config.minQuadCount) {
        maxCount = set.site->config.minQuadCount;
      } else if (maxCount > set.site->config.maxQuadCount) {
        maxCount = set.site->config.maxQuadCount;
      }

      if (maxCount >= set.vlist.size()) {
        endSiteCount += set.vlist.size();
        continue;
      }

      if (maxCount == 0) {
        set.vlist.clear();
        set.rlist.clear();
        continue;
      }

      // create separate x and y vectors from our vertex vector
      // as required by the kmeans function
      std::vector<double> x;
      std::vector<double> y;
      for (unsigned int v = 0; v < set.vlist.size(); ++v) {
        x.push_back(set.vlist[v].x);
        y.push_back(set.vlist[v].y);
      }

      // kmeans will return a vector the same size as the source x and y
      // vectors which will indicate which vertexes should be kept and
      // which discarded based on the K value supplied (maxquadcount)
      keep = Kmeans(x, y, static_cast<int>(maxCount), true);
      if (keep.size() == 0)
        continue;
    } else {
      // otherwise randomly drop percent of points based on
      // loopcount/MAX_LOOP_COUNT

      if (loopcount == 0) {
        endSiteCount += set.vlist.size();
        continue;
      }

      // make sure the keep ratio never gets to 0
      double ratio;
      if (loopcount < MAX_LOOP_COUNT) {
        ratio = 1.0 - (loopcount * 1.0 / MAX_LOOP_COUNT);
      } else {
        ratio = .02;
      }

      unsigned int keepcount = uint(ratio * set.vlist.size());

      int dupCount = 0;

      while (keep.size() < keepcount) {
        // get a new random id, wrapping it at the size of the vertex list
        int next_id = random() % set.vlist.size();

        // search for this new id to see if we've already choosen it
        std::vector<int>::iterator found =
            find(keep.begin(), keep.end(), next_id);

        // if it is unique, add it to our keep list
        if (found == keep.end()) {
          keep.push_back(next_id);
        } else {
          ++dupCount;
        }
      }
    }

    // replace the original vertex list with the keepers
    // track the discarded vertexes in order to re-insert them a level lower
    VertexList new_vlist;
    RecordList new_rlist;
    std::vector<int>::iterator keeper = keep.begin();
    VertexList::iterator vert = set.vlist.begin();
    RecordList::iterator label = set.rlist.begin();
    int count = 0;
    for (; keeper != keep.end(); ++keeper, ++vert, ++label) {
      if (*keeper) {
        count++;
        new_vlist.push_back(*vert);
        new_rlist.push_back(*label);
      }
    }

    set.vlist.swap(new_vlist);
    set.rlist.swap(new_rlist);
    endSiteCount += set.vlist.size();
  }

  // for sites numfeatures == numvertices
  the_selector_->UpdateSimplifyStats(beginSiteCount - endSiteCount,
                                     beginSiteCount - endSiteCount, 0);
  // Return true if we actually simplified the set.
  return beginSiteCount > endSiteCount;
}

QStringList gstLayer::GetExternalContextScripts(void) const {
  // TODO: - find reference external scripts and add them to this list
  return QStringList();
}



// ----------------------------------------------------------------------------

gstLevelState::gstLevelState() {
  reset();
}

void gstLevelState::reset() {
  memset(_states, 0, NUM_LEVELS * sizeof(int));
}

gstLevelState &gstLevelState::operator=(const gstLevelState& that) {
  memcpy(_states, that._states, NUM_LEVELS * sizeof(int));
  return *this;
}

bool gstLevelState::isEmpty() const {
  for (unsigned int lev = 0; lev < NUM_LEVELS; ++lev) {
    if (_states[lev] != 0)
      return false;
  }
  return true;
}
