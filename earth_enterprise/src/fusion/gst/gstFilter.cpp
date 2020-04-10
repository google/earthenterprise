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


#include <gstFilter.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "GL/gl.h"

#include <gstSource.h>
#include <gstSite.h>
#include <gstGeode.h>
#include <gstSelectRule.h>
#include <gstFileUtils.h>
#include <khGuard.h>
#include <gstSourceManager.h>
#include <gstGeometry.h>
#include <gstJobStats.h>
#include <gstLayer.h>
#include <autoingest/.idl/storage/VectorFuseAssetConfig.h>


gstFilter::gstFilter(const DisplayRuleConfig &disprule_config, int id)
    : name_(disprule_config.name),
      config_(disprule_config.filter),
      id_(id),
      feature_configs_(FuseFeatureConfig(disprule_config.feature),
               gstFeaturePreviewConfig(disprule_config.feature)),
      site_(FuseSiteConfig(disprule_config.site),
            gstSitePreviewConfig(disprule_config.site)),
      geo_index_(gstGeoIndexImpl::Create())
{
  PopulateSelectRules();
}


gstFilter::gstFilter(const FilterConfig &filter_config, int id)
    : name_(),
      config_(filter_config),
      id_(id),
      feature_configs_(FuseFeatureConfig(), gstFeaturePreviewConfig()),
      site_(FuseSiteConfig(), gstSitePreviewConfig()),
      geo_index_(gstGeoIndexImpl::Create())
{
  PopulateSelectRules();
}


gstFilter::gstFilter(const FuseItem &fuse_item, int id)
    : name_(),
      config_(),
      id_(id),
      feature_configs_(fuse_item.feature, gstFeaturePreviewConfig()),
      site_(fuse_item.site, gstSitePreviewConfig()),
      geo_index_(gstGeoIndexImpl::Create())
{
  PopulateSelectRules();
}


void gstFilter::PopulateSelectRules(void) {
  for (std::vector<SelectRuleConfig>::const_iterator it
         = config_.selectRules.begin();
       it != config_.selectRules.end(); ++it) {
    select_rules_.append(new gstSelectRule(*it));
  }
}


gstFilter::~gstFilter() {
  ClearSelectRules();
}

void gstFilter::Reset() {
  geo_index_->Reset();
}

void gstFilter::Finalize() {
  geo_index_->Finalize();
}

bool gstFilter::ReadQueryResultsFile(const std::string& path, int source_id) {
  return geo_index_->ReadFile(path, source_id);
}

bool gstFilter::WriteQueryResultsFile(const std::string& path) {
  return geo_index_->WriteFile(path);
}

bool gstFilter::IsVisible(const gstDrawState& state) const {
  const double effective_level = state.level + state.offset;
  return !(effective_level < feature_configs_.config.minLevel ||
           effective_level > feature_configs_.config.maxLevel);
}


void gstFilter::DrawSelectedFeatures(const gstDrawState &state, int srcid,
                                     int layer, int* max_count) {
  gstBBox cull_box = state.cullbox;
  if (state.IsMercatorPreview()) {
    // cull_box needs to revert to Flat Projection normalized coordinate as
    // bounding box of geo_index is in Flat Projection normalized coordinate.
    MercatorProjection::NormalizeToFlatFromMercator(&cull_box.n, &cull_box.s);
  }

  if (!cull_box.Intersect(GetBoundingBox()))
    return;

  // get a list of all features that are within our box
  std::vector<int> feature_list;
  std::vector<gstBBox> grid_boxes;
  geo_index_->Intersect(cull_box, &feature_list, &grid_boxes);

  if (state.mode & DrawBBoxes) {
    glColor3f(0.0, 1.0, 1.0);
    glLineWidth(1);
    for (std::vector<gstBBox>::iterator it = grid_boxes.begin();
         it != grid_boxes.end(); ++it) {
      gstBBox& b = *it;
      DRAW_BOX_2D(b.w - state.frust.w, b.e - state.frust.w,
                  b.s - state.frust.s, b.n - state.frust.s);
    }
  }

  // GetGeomertyOrThrow could theorectically throw, but the fact that we
  // already got the features and box in order to select these features,
  // it is EXTREMELY unlikely that it would not fail
  // Since it is SO unlikely and since we're in the middle of the draw (where
  // an error dialog is very bad), just silently ignore any errors.
  try {
    for (std::vector<int>::iterator it = feature_list.begin();
         it != feature_list.end(); ++it) {
      gstGeometryHandle gh = theSourceManager->GetGeometryOrThrow(
          UniqueFeatureId(srcid, layer, *it), state.IsMercatorPreview());

      gh->Draw(state.frust, feature_configs_.preview_config);

      *max_count = *max_count - 1;
      if (*max_count <= 0)
        break;
    }
  } catch (...) {
    // silently ignore errors. See comment before the 'try'
  }
}

void gstFilter::IntersectBBox(const gstBBox& b, std::vector<int>* feature_list) {
  geo_index_->Intersect(b, feature_list);
}

bool gstFilter::FeatureEnabled() const {
  return feature_configs_.config.enabled();
}

bool gstFilter::SiteEnabled() const {
  return site_.config.enabled;
}


void gstFilter::ClearSelectRules() {
  for (unsigned int ii = 0; ii < select_rules_.length(); ++ii)
    delete select_rules_[ii];

  select_rules_.clear();
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_QUERY, JOBSTATS_GETATTR, JOBSTATS_EVAL,
      JOBSTATS_GETBOX, JOBSTATS_NOMATCH, JOBSTATS_INSERT};
static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_QUERY,   "Query Total     "},
  {JOBSTATS_GETATTR, "Get Attributes  "},
  {JOBSTATS_EVAL,    "Evaluate Filter "},
  {JOBSTATS_GETBOX,  "Get Feature Box "},
  {JOBSTATS_NOMATCH, "No Match        "},
  {JOBSTATS_INSERT,  "Index Insert    "}
};
gstJobStats* apply_stats = new gstJobStats("FILTER", JobNames, 6);
#endif

// XXX someday we might suport filtering on the
// XXX geographic area, perimiter, bounding box, etc.
void gstFilter::ThrowingTryApply(const gstRecordHandle &rec,
                                 const UniqueFeatureId &featureId,
                                 const gstRecordJSContext &cx,
                                 const KHJSScript &matchScript,
                                 bool &have_match,
                                 SoftErrorPolicy &soft_errors) {
  // start query
  JOBSTATS_SCOPED(apply_stats, JOBSTATS_QUERY);

  have_match = false;
  if (MatchLogic() == FilterConfig::JSExpression) {
    if (cx && matchScript) {
      JOBSTATS_SCOPED(apply_stats, JOBSTATS_EVAL);
      QString error;
      if (!cx->TryExecuteScript(matchScript, have_match, error)) {
        throw khException(
            kh::tr("Error evaluating JS expression for filter:\n%1")
            .arg(error));
      }
    } else {
      // empty expression matches everything
      // this matches what happens if you have another match logic selected
      // and don't supply any rules
      have_match = true;
    }
  } else {
    if (select_rules_.length() == 0) {
      have_match = true;
    } else {
      if (!rec) {
        throw khException(
            kh::tr("Internal Error: Trying to apply filter to null record"));
      }

      JOBSTATS_BEGIN(apply_stats, JOBSTATS_GETATTR);
      JOBSTATS_END(apply_stats, JOBSTATS_GETATTR);

      JOBSTATS_BEGIN(apply_stats, JOBSTATS_EVAL);
      // select rule without rec means we fail
      if (rec->IsEmpty()) {
        have_match = false;
      } else {
        // select rule and rec needs to be tested
        for (unsigned int ii = 0; ii < select_rules_.length(); ++ii) {
          gstSelectRule* rule = select_rules_[ii];
          if (rule->eval(rec)) {
            have_match = true;
            if (MatchLogic() == FilterConfig::MatchAny)
              break;
          } else if (MatchLogic() == FilterConfig::MatchAll) {
            have_match = false;
            break;
          }
        }
      }
      JOBSTATS_END(apply_stats, JOBSTATS_EVAL);
    }
  }

  // we have a match, now confirm the feature is valid!
  // this is probably not necessary for assets since they only contain valid features
  if (have_match) {
    try {
      JOBSTATS_SCOPED(apply_stats, JOBSTATS_GETBOX);
      gstBBox box(theSourceManager->GetFeatureBoxOrThrow(featureId));

      if (!box.Valid()) {
        throw khSoftException(kh::tr("Invalid bounding box for feature %1")
                              .arg(featureId.feature_id));
      }

      static const gstBBox kMaxDomain(0.0, 1.0, 0.25, 0.75);

      if (!kMaxDomain.Contains(box)) {
        throw khSoftException(QString().sprintf(
                                  "rejecting invalid feature (%d)"
                                  "(n:%6.6lg s:%6.6lg w:%6.6lg e:%6.6lg)",
                                  featureId.feature_id,
                                  khTilespace::Denormalize(box.n),
                                  khTilespace::Denormalize(box.s),
                                  khTilespace::Denormalize(box.w),
                                  khTilespace::Denormalize(box.e)));
      }

      JOBSTATS_BEGIN(apply_stats, JOBSTATS_INSERT);
      geo_index_->Insert(featureId.feature_id, box);
      JOBSTATS_END(apply_stats, JOBSTATS_INSERT);
    } catch (const khSoftException &e) {
      have_match = false;
      soft_errors.HandleSoftError(e.qwhat());
    }
  } else {
    JOBSTATS_BEGIN(apply_stats, JOBSTATS_NOMATCH);
    JOBSTATS_END(apply_stats, JOBSTATS_NOMATCH);
  }
}

bool gstFilter::ThrowingTryHasMatch(const gstRecordHandle &rec,
                                 const UniqueFeatureId &featureId,
                                 const gstRecordJSContext &cx,
                                 const KHJSScript &matchScript,
                                 SoftErrorPolicy &soft_errors) {
  // start query
  JOBSTATS_SCOPED(apply_stats, JOBSTATS_QUERY);

  bool have_match = false;
  if (MatchLogic() == FilterConfig::JSExpression) {
    if (cx && matchScript) {
      JOBSTATS_SCOPED(apply_stats, JOBSTATS_EVAL);
      QString error;
      if (!cx->TryExecuteScript(matchScript, have_match, error)) {
        throw khException(
            kh::tr("Error evaluating JS expression for filter:\n%1")
            .arg(error));
      }
    } else {
      // empty expression matches everything
      // this matches what happens if you have another match logic selected
      // and don't supply any rules
      have_match = true;
    }
  } else {
    if (select_rules_.length() == 0) {
      have_match = true;
    } else {
      if (!rec) {
        throw khException(
            kh::tr("Internal Error: Trying to apply filter to null record"));
      }

      JOBSTATS_BEGIN(apply_stats, JOBSTATS_GETATTR);
      JOBSTATS_END(apply_stats, JOBSTATS_GETATTR);

      JOBSTATS_BEGIN(apply_stats, JOBSTATS_EVAL);
      // select rule without rec means we fail
      if (rec->IsEmpty()) {
        have_match = false;
      } else {
        // select rule and rec needs to be tested
        for (unsigned int ii = 0; ii < select_rules_.length(); ++ii) {
          gstSelectRule* rule = select_rules_[ii];
          if (rule->eval(rec)) {
            have_match = true;
            if (MatchLogic() == FilterConfig::MatchAny)
              break;
          } else if (MatchLogic() == FilterConfig::MatchAll) {
            have_match = false;
            break;
          }
        }
      }
      JOBSTATS_END(apply_stats, JOBSTATS_EVAL);
    }
  }
  return have_match;
}
