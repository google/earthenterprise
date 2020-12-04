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

#ifndef FUSION_FUSIONUI_ASSETDERIVEDIMPL_H__
#define FUSION_FUSIONUI_ASSETDERIVEDIMPL_H__

#include "AssetDerived.h"
#include "AssetDisplayHelper.h"
#include "fusion/fusionui/ResetConfigIdsExecutor.h"


template <class Defs, class FinalClass>
const FinalClass* AssetDerived<Defs, FinalClass>::final(void) const {
  return static_cast<const FinalClass*>(this);
}

template <class Defs, class FinalClass>
FinalClass* AssetDerived<Defs, FinalClass>::final(void) {
  return static_cast<FinalClass*>(this);
}

template <class Defs, class FinalClass>
AssetDerived<Defs, FinalClass>::AssetDerived(QWidget *parent) :
    AssetBase(parent),
    saved_edit_request_(FinalClass::FinalMakeNewRequest()) {
  saved_edit_request_.assetname = AssetBase::untitled_name.toUtf8().constData();
  Init();
}

template <class Defs, class FinalClass>
AssetDerived<Defs, FinalClass>::AssetDerived(
    QWidget *parent, const typename Defs::Request &req) :
    AssetBase(parent),
    saved_edit_request_(req) {
  Init();
}

template <class Defs, class FinalClass>
AssetDefs::Type AssetDerived<Defs, FinalClass>::AssetType() const {
  return AssetDisplayHelper::AssetType(Defs::kAssetDisplayKey);
}

template <class Defs, class FinalClass>
std::string AssetDerived<Defs, FinalClass>::AssetSubtype() const {
  return AssetDisplayHelper::AssetSubType(Defs::kAssetDisplayKey);
}

template <class Defs, class FinalClass>
QString AssetDerived<Defs, FinalClass>::AssetPrettyName() const {
  return AssetDisplayHelper::PrettyName(Defs::kAssetDisplayKey);
}

template <class Defs, class FinalClass>
QWidget* AssetDerived<Defs, FinalClass>::BuildMainWidget(QWidget* parent) {
  main_widget_ = new typename Defs::Widget(parent, this);
  return main_widget_;
}

template <class Defs, class FinalClass>
bool AssetDerived<Defs, FinalClass>::IsModified() {
  try {
    typename Defs::Request new_request(AssembleEditRequest());
    bool modified = new_request != saved_edit_request_;
    if (modified && (getNotifyLevel() >= NFY_VERBOSE)) {
      fprintf(stderr, "---------- OLD ----------\n");
      std::string oldstr;
      saved_edit_request_.SaveToString(oldstr, "DEBUG_OLD");
      fprintf(stderr, "%s", oldstr.c_str());
      fprintf(stderr, "---------- NEW ----------\n");
      std::string newstr;
      new_request.SaveToString(newstr, "DEBUG_NEW");
      fprintf(stderr, "%s", newstr.c_str());
      fprintf(stderr, "-------------------------\n");
    }
    return modified;
  } catch (...) {
    // Don't report any errors. Just say that it is modified.  Callers will
    // rediscover the errors on their own.  If we don't do this, we end up
    // with the potential for double messages being sent to the user.
    //
    // This count is for future code instrumentation
    UnhandledErrorCount++;
  }
  return true;
}

template <class Defs, class FinalClass>
  bool AssetDerived<Defs, FinalClass>::SubmitEditRequest(QString* error_msg, bool last_save_error) {
  typename Defs::Request request = AssembleEditRequest();
  if (!last_save_error && request == saved_edit_request_ )
    return true;

  if (last_save_error || request.assetname != saved_edit_request_.assetname) {
    // It is a "SaveAs"'s edit request, or earlier request was not
    // successfully saved, so resetting IDs in fuse config to force
    // generating of new ones.
    ResetConfigIds(&request);
    // Update model-object: sync with locally modified request.
    ReInit(request);
    // Re-assemble request after updating model-object.
    request = AssembleEditRequest();
  }


  if (Defs::kSubmitFunc(request, *error_msg, 0 /* timeout */)) {
    // TODO: To be consistent, the model (gstVectorProject-object)
    // may be synced from storage after submitting. For some assets, a config
    // may be modified in the systemmanager (restoring asset_uuid, fuid_*,
    // indexVersion from previous version) - a "SaveAs" use-case when re-writing
    // an existing asset.
    saved_edit_request_ = request;
    return true;
  } else {
    return false;
  }
}

template <class Defs, class FinalClass>
void AssetDerived<Defs, FinalClass>::Init(bool re_init) {
  // defer to final class
  final()->FinalPreInit();

  if (re_init == false) {
    InstallMainWidget();

    SetName(saved_edit_request_.assetname.c_str());
    SetMeta(saved_edit_request_.meta);

    main_widget_->Prefill(saved_edit_request_);
    UnhandledErrorCount = 0;
  }

  // defer to final class
  final()->FinalExtraInit();

  RestoreLayout();
}

template <class Defs, class FinalClass>
typename Defs::Request
AssetDerived<Defs, FinalClass>::AssembleEditRequest(void) {
  typename Defs::Request request = saved_edit_request_;
  request.assetname = Name().toUtf8().constData();
  request.meta = Meta();

  main_widget_->AssembleEditRequest(&request);

  return request;
}

template <class Defs, class FinalClass>
typename Defs::Request
AssetDerived<Defs, FinalClass>::FinalMakeNewRequest(void) {
  return typename Defs::Request();
}

template <class Defs, class FinalClass>
void AssetDerived<Defs, FinalClass>::ResetConfigIds(
    typename Defs::Request *const request) {
  ResetConfigIdsExecutor::Exec(request);
}

template <class Defs, class FinalClass>
void AssetDerived<Defs, FinalClass>::ReInit(
    const typename Defs::Request &request) {
  saved_edit_request_ = request;
  Init(true);
}

#endif // FUSION_FUSIONUI_ASSETDERIVEDIMPL_H__
