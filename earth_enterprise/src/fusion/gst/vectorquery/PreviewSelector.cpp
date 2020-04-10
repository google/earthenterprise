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

#include "PreviewSelector.h"
#include <khException.h>
#include <gstSelectRule.h>
#include <gstSourceManager.h>
#include <gstProgress.h>
#include <gstSource.h>

namespace vectorquery {


// ****************************************************************************
// ***  RecordFilter
// ****************************************************************************
RecordFilter::RecordFilter(const FilterConfig &cfg) :
    config(cfg),
    geoIndex(gstGeoIndexImpl::Create()),
    currentCount(0)
{
  // populate select rules from config
  if (config.selectRules.size() > 0) {
    selectRules.reserve(config.selectRules.size());
    for (std::vector<SelectRuleConfig>::const_iterator it
           = config.selectRules.begin(); it != config.selectRules.end();
         ++it) {
      selectRules.push_back(new gstSelectRule(*it));
    }
  }
}

RecordFilter::~RecordFilter(void)
{
}

void
RecordFilter::Reset(void)
{
  currentCount = 0;
  geoIndex->Reset();
}

// true on match, false on no match, exception on error
bool
RecordFilter::TryApply(const gstRecordHandle &rec,
                       const UniqueFeatureId &featureId,
                       const gstRecordJSContext &cx,
                       const KHJSScript &matchScript,
                       SoftErrorPolicy &soft_errors)
{
  bool have_match = false;

  if (config.match == FilterConfig::JSExpression) {
    if (cx && matchScript) {
      QString error;
      if (!cx->TryExecuteScript(matchScript, have_match, error)) {
        throw khException
          (kh::tr("Error evaluating JS expression for filter:\n%1")
           .arg(error));
      }
    } else {
      // empty expression matches everything
      // this matches what happens if you have another match logic selected
      // and don't supply any rules
      have_match = true;
    }
  } else {
    if (selectRules.size() == 0) {
      have_match = true;
    } else {
      if (!rec) {
        throw khException(
            kh::tr("Internal Error: Trying to apply filter to null record"));
      }

      // select rule without rec means no match
      if (rec->IsEmpty()) {
        have_match = false;
      } else {
        // select rule and rec needs to be tested
        for (unsigned int ii = 0; ii < selectRules.size(); ++ii) {
          gstSelectRule* rule = selectRules[ii];
          if (rule->eval(rec)) {
            have_match = true;
            if (config.match == FilterConfig::MatchAny)
              break;
          } else if (config.match == FilterConfig::MatchAll) {
            have_match = false;
            break;
          }
        }
      }
    }
  }

  // We have a match, now confirm the feature is valid!  This is probably
  // not necessary for assets since they only contain valid features
  if (have_match) {
    try {
      gstBBox box(theSourceManager->GetFeatureBoxOrThrow(featureId));

      if (!box.Valid()) {
        throw khSoftException(kh::tr("Invalid bounding box for feature %1")
                              .arg(featureId.feature_id));
      }

      if (!gstBBox(0.0, 1.0, 0.25, .75).Contains(box)) {
        throw khSoftException(QString().sprintf(
                                  "rejecting invalid feature "
                                  "(n:%6.6lg s:%6.6lg w:%6.6lg e:%6.6lg)",
                                  box.n * 360 - 180, box.s * 360 - 180,
                                  box.w * 360 - 180, box.e * 360 - 180));
      }

      geoIndex->Insert(featureId.feature_id, box);
      ++currentCount;
    } catch (const khSoftException &e) {
      have_match = false;
      soft_errors.HandleSoftError(e.qwhat());
    }
  }

  return have_match;
}

void
RecordFilter::Finalize(void)
{
  geoIndex->Finalize();
}

bool
RecordFilter::ReadQueryResultsFile(const std::string& path,
                                   const gstSharedSource &source)
{
  return geoIndex->ReadFile(path, source);
}

std::uint32_t
RecordFilter::Intersect(const gstBBox& b, std::vector<int>* match_list) {
  geoIndex->Intersect(b, match_list);
  return match_list->size();
}


// ****************************************************************************
// ***  PreviewSelector
// ****************************************************************************
PreviewSelector::PreviewSelector
(const gstSharedSource &source_,
 const QueryConfig &config_,
 const std::vector<geMultiRange<std::uint32_t> > &validLevels_,
 const khTilespace &tilespace_,
 double oversizeFactor_) :
    config(config_),
    validLevels(validLevels_),
    source(source_),
    tilespace(tilespace_),
    oversizeFactor(oversizeFactor_)
{
  filters.reserve(config.filters.size());
  for (unsigned int f = 0; f < config.filters.size(); ++f) {
    filters.push_back(new RecordFilter(config.filters[f]));
  }
}

void
PreviewSelector::ApplyQueries(gstProgress &guiProgress,
                              SoftErrorPolicy &soft_errors) {
  guiProgress.SetVal(0);


  // figure out if we have JS expressions for any of our filters
  bool wantJS = false;
  for (unsigned int f = 0; f < filters.size(); ++f) {
    if (config.filters[f].HasJS()) {
      wantJS = true;
      break;
    }
  }

  // Reset the filters
  for (unsigned int f = 0; f < filters.size(); ++f) {
    filters[f]->Reset();
  }

  //---------------------------------------------------------------------------
  // iterate through every geode of source file, comparing with
  // each filter sequentially
  //---------------------------------------------------------------------------
  int gnum = source->NumFeatures(0 /* layerId */);
  if (gnum == 0) {
    throw khException(kh::tr("Source %1 appears to be empty or corrupt.\n"
                             "Feature count is 0.")
                      .arg(source->name()));
  }

  // create the JS context and compile the match scripts
  gstHeaderHandle recordHeader = source->GetAttrDefs(0 /* only layer */);
  if (!recordHeader) {
    throw khException(kh::tr("Unable to retrieve source header"));
  }
  gstRecordJSContext cx;
  std::vector<KHJSScript> matchScripts(filters.size());
  if (wantJS) {
    try {
      // my context script will already have all of the levels of
      // context scripts collapsed into it.
      QStringList contextScripts;
      if (!config.contextScript.isEmpty()) {
        contextScripts.append(config.contextScript);
      }
      cx = gstRecordJSContextImpl::Create(recordHeader, contextScripts);
      for (unsigned int f = 0; f < filters.size(); ++f) {
        if (config.filters[f].match == FilterConfig::JSExpression) {
          matchScripts[f] = cx->CompileScript(config.filters[f].matchScript,
                                              QString("Filter Expression %1")
                                              .arg(f+1));
        }
      }
    } catch (const khException &e) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg(e.qwhat()));
    } catch (const std::exception &e) {
      throw khException(kh::tr("Javascript compilation error: %1")
                        .arg(e.what()));
    } catch (...) {
      throw khException(kh::tr("Javascript compilation error: Unknown"));
    }
  }

  notify(NFY_DEBUG, "Source %s has %d features for layer %d",
         source->name(), gnum, 0 /* layerId */);
  double step = 100.0 / static_cast<double>(gnum);
  for (int fnum = 0; fnum < gnum; ++fnum) {
    // has the user tried to interrupt?
    if (guiProgress.GetState() == gstProgress::Interrupted) {
      return;
    }

    // if we have any fields, we need to fetch the record
    gstRecordHandle recordHandle;
    if (recordHeader->numColumns() > 0) {
      recordHandle = source->GetAttributeOrThrow(0 /* layerId */, fnum);
    }

    if (cx) {
      cx->SetRecord(recordHandle);
    }

    for (unsigned int f = 0; f < filters.size(); ++f) {
      bool match =
        filters[f]->TryApply(recordHandle,
                             UniqueFeatureId(source->Id(), 0, fnum),
                             cx,  matchScripts[f],
                             soft_errors);

      // see if features can be shared across filters
      if (match && !config.allowFeatureDuplication) {
        break;
      }
    }

    guiProgress.SetVal(static_cast<int>(static_cast<double>(fnum) * step));
  }

  guiProgress.SetVal(100);

  // after queries are complete, rebuild geoindex
  for (unsigned int f = 0; f < filters.size(); ++f) {
    filters[f]->Finalize();
  }
}


bool
PreviewSelector::Fetch(WorkflowOutputTile *out) {
  assert(filters.size() == out->displayRules.size());

  // populate normTargetExtents for this tile
  std::uint32_t level, row, col;
  out->path.GetLevelRowCol(&level, &row, &col);

  khExtents<double> norm_extents =
    khTileAddr(level, row, col).normExtents(tilespace);
  gstBBox normTargetExtents(norm_extents.west(),
                            norm_extents.east(),
                            norm_extents.south(),
                            norm_extents.north());

  // Ask each of my filters to intersect with the target extents
  std::uint32_t count = 0;
  for (unsigned int f = 0; f < filters.size(); ++f) {
    if (validLevels[f].Contains(level)) {
      count += filters[f]->Intersect(normTargetExtents,
                                     &out->displayRules[f].selectedIds);
    }
  }

  notify(NFY_VERBOSE, "PreviewSelector lrc(%u,%u,%u): count = %u",
         level, row, col, count);
  return (count > 0);
}



} // namespace vectorquery
