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

//

#ifndef FUSION_FUSIONUI_ASSETDERIVED_H__
#define FUSION_FUSIONUI_ASSETDERIVED_H__

#include "AssetBase.h"


template <class Defs, class FinalClass>
class AssetDerived : public AssetBase {
 public:
  AssetDerived(QWidget *parent);
  AssetDerived(QWidget *parent, const typename Defs::Request &req);

 protected:
  // ***** designed to be implemented in final *****
  // Final should on define them if default implmentation is not
  // sufficient. These are not virtual. The final version is called using
  // typecasting and function shadowing.

  // default constructs from void
  static typename Defs::Request FinalMakeNewRequest(void);

  // default does nothing
  void FinalPreInit(void)   { }
  void FinalExtraInit(void) { }

  // *** inherited from AssetBase *****
  // Should NOT be reimplemented in FinalClass
  virtual AssetDefs::Type AssetType() const;
  virtual std::string AssetSubtype() const;
  virtual QString AssetPrettyName() const;
  virtual QWidget* BuildMainWidget(QWidget* parent);
  virtual bool IsModified();
  virtual bool SubmitEditRequest(QString* error_msg, bool save_error_);

  typename Defs::Request saved_edit_request_;
  typename Defs::Widget* main_widget_;

 private:
  void Init(bool re_init = false);
  typename Defs::Request AssembleEditRequest(void);
  int UnhandledErrorCount;

  // Reset IDs(asset_uuid, fuid_resource, fuid_channel, indexVersion) in fuse
  // config. It is used to support unique IDs for assets in system - "SaveAs"
  // use-case.
  void ResetConfigIds(typename Defs::Request *const request);

  // Updates model-object to sync with locally modified request.
  void ReInit(const typename Defs::Request &request);

  const FinalClass* final(void) const;
  FinalClass* final(void);
};

#endif // FUSION_FUSIONUI_ASSETDERIVED_H__
