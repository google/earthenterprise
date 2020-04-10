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


#include <gstFeature.h>
#include <gstValue.h>
#include <gstLimits.h>
#include <gstFilter.h>
#include <gstRecord.h>
#include <gstFormatRules.h>
#include <gstJobStats.h>
#include <JSDisplayBundle.h>

gstFeatureConfigs::gstFeatureConfigs(
    const FuseFeatureConfig& cfg,
    const gstFeaturePreviewConfig &preview_cfg)
    : config(cfg),
      preview_config(preview_cfg),
      label_fmt_(NULL),
      custom_height_fmt_(NULL) {
  feature_header_ = gstHeaderImpl::Create();
  feature_header_->addSpec("label", gstTagUnicode);
  feature_header_->addSpec("customHeight", gstTagDouble);
}

gstFeatureConfigs::~gstFeatureConfigs() {
  delete label_fmt_;
  delete custom_height_fmt_;
}

bool gstFeatureConfigs::AttributeExpansionNeeded() const {
  if (config.drawAsRoads && label().length()) {
    return true;
  } else if (config.style.altitudeMode != StyleConfig::ClampToGround &&
             config.style.enableCustomHeight &&
             config.style.customHeightVariableName.length()) {
    return true;
  } else {
    return false;
  }
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_EXPAND, JOBSTATS_ROADLABEL, JOBSTATS_FORMAT,
      JOBSTATS_CLEANUP, JOBSTATS_ALTITUDE};
static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_EXPAND,    "Expand       "},
  {JOBSTATS_ROADLABEL, "Road Label   "},
  {JOBSTATS_FORMAT,    "+ Format     "},
  {JOBSTATS_CLEANUP,   "+ Cleanup    "},
  {JOBSTATS_ALTITUDE,  "Altitude     "}
};

gstJobStats* feature_stats = new gstJobStats("Feature", JobNames, 6);
#endif

// must supply empty record for any feature that
// doesn't need attribute expansion
gstRecordHandle gstFeatureConfigs::DummyExpand() const {
  return feature_header_->NewRecord();
}

// fill in two fields of the feature record
//   0 : road name
//   1 : custom height value
gstRecordHandle gstFeatureConfigs::Expand(gstRecordHandle src_rec,
                                   JSDisplayBundle &jsbundle,
                                   unsigned int filterId) const {
  JOBSTATS_SCOPED(feature_stats, JOBSTATS_EXPAND);
  gstHeaderHandle src_hdr = src_rec->Header();
  gstRecordHandle new_rec = feature_header_->NewRecord();

  JOBSTATS_BEGIN(feature_stats, JOBSTATS_ROADLABEL);
  if (config.drawAsRoads) {
    if (label().length()) {
      JOBSTATS_BEGIN(feature_stats, JOBSTATS_FORMAT);
      QString outval;

      if (jsbundle.displayJS[filterId].roadLabelScript) {
        jsbundle.cx->SetRecord(src_rec);
        QString error;
        if (!jsbundle.cx->TryExecuteScript
            (jsbundle.displayJS[filterId].roadLabelScript, outval, error)) {
          notify(NFY_WARN, "Error executing JS: %s",
                 (const char *)error.utf8());
          outval = QString();
          return gstRecordHandle();
        }
      } else {
        if (label_fmt_ == NULL)
          label_fmt_ = new gstRecordFormatter(label(), src_hdr);

        // strip white space at beginning and end
        // replace any sequence of white space with a single space.
        outval = label_fmt_->out(src_rec).simplifyWhiteSpace();
      }
      JOBSTATS_END(feature_stats, JOBSTATS_FORMAT);

      JOBSTATS_BEGIN(feature_stats, JOBSTATS_CLEANUP);
      // convert all strings to lowercase, then make first char of each word upper
      if (!outval.isEmpty() && reformatLabel()) {
        outval = theFormatRules->reformat(outval);
      }
      JOBSTATS_END(feature_stats, JOBSTATS_CLEANUP);

      if (!outval.isEmpty())
        new_rec->Field(0)->set(outval);
    }
  }
  JOBSTATS_END(feature_stats, JOBSTATS_ROADLABEL);

  // get the custom height variable field
  JOBSTATS_BEGIN(feature_stats, JOBSTATS_ALTITUDE);
  if (config.style.altitudeMode != StyleConfig::ClampToGround &&
      config.style.enableCustomHeight &&
      config.style.customHeightVariableName.length()) {
    if (custom_height_fmt_ == NULL)
      custom_height_fmt_ =
        new gstRecordFormatter(config.style.customHeightVariableName,
                               src_hdr);
    QString outval = custom_height_fmt_->out(src_rec);
    if (outval.isEmpty()) {
      new_rec->Field(1)->set(QString("0"));
    } else {
      new_rec->Field(1)->set(outval);
    }
  } else {
    new_rec->Field(1)->set(QString("0"));
  }
  JOBSTATS_END(feature_stats, JOBSTATS_ALTITUDE);

  return new_rec;
}


QString gstFeatureConfigs::PrettyFeatureType() const {
  return VectorDefs::PrettyFeatureDisplayType(config.featureType);
}
