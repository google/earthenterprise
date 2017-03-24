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

#ifndef KHSRC_FUSION_GST_GSTFILTER_H__
#define KHSRC_FUSION_GST_GSTFILTER_H__

#include <vector>

#include <gstGeoIndex.h>
#include <khArray.h>
#include <gstLimits.h>
#include <gstBBox.h>
#include <gstCallback.h>
#include <gstFeature.h>
#include <gstSite.h>
#include <gstRecordJSContext.h>
#include <SoftErrorPolicy.h>

class DisplayRuleConfig;
class FilterConfig;
class FuseItem;
class UniqueFeatureId;
class gstSelectRule;
class gstDrawState;

class gstFilter {
 public:
  gstFilter(const DisplayRuleConfig &disprule_config, int id);
  gstFilter(const FilterConfig &filter_config, int id);
  gstFilter(const FuseItem &fuse_item, int id);
  ~gstFilter();

  void ThrowingTryApply(const gstRecordHandle &rec,
                        const UniqueFeatureId &featureId,
                        const gstRecordJSContext &cx,
                        const KHJSScript &matchScript,
                        bool &have_match,
                        SoftErrorPolicy &soft_errors);
  // This call is similar to ThrowingTryApply, but simply applies the matching
  // rules and returns true if the featureId has a match.
  bool ThrowingTryHasMatch(const gstRecordHandle &rec,
                           const UniqueFeatureId &featureId,
                           const gstRecordJSContext &cx,
                           const KHJSScript &matchScript,
                           SoftErrorPolicy &soft_errors);

  const QString& Name() const { return name_; }

  int Id() const { return id_; }

  const gstFeatureConfigs& FeatureConfigs() const { return feature_configs_; }
  bool FeatureEnabled() const;

  const gstSite& Site() const { return site_; }
  bool SiteEnabled() const;

  FilterConfig::MatchType MatchLogic() const { return config_.match; }

  void DrawSelectedFeatures(const gstDrawState& state, int srcid, int layer,
                            int* max_count);
  void Reset();
  void Finalize();

  bool ReadQueryResultsFile(const std::string& path, int source_id);
  bool WriteQueryResultsFile(const std::string& path);

  gstBBox GetBoundingBox() const { return geo_index_->GetBoundingBox(); }
  void IntersectBBox(const gstBBox&, std::vector<int>* feature_list);
  int GetSelectListSize() const { return geo_index_->GetCount(); }

  bool IsVisible(const gstDrawState& s) const;

  gstGeoIndexHandle GetGeoIndex() const { return geo_index_; }
 private:
  friend class gstSelector;

  void SetGeoIndex(gstGeoIndexHandle index) { geo_index_ = index; }
  void PopulateSelectRules(void);

  khArray<gstSelectRule*> SelectRules() const { return select_rules_; }
  void ClearSelectRules();

  QString name_;

  const FilterConfig config_;

  int id_;

  gstFeatureConfigs feature_configs_;
  gstSite site_;

  khArray< gstSelectRule* > select_rules_;

  gstGeoIndexHandle geo_index_;
};

#endif  // !KHSRC_FUSION_GST_GSTFILTER_H__
