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


#ifndef KHSRC_FUSION_FUSIONUI_VECTORPROJECT_H__
#define KHSRC_FUSION_FUSIONUI_VECTORPROJECT_H__

#include "AssetDerived.h"
#include "ProjectWidget.h"
#include "AssetDisplayHelper.h"
#include <autoingest/.idl/storage/VectorProjectConfig.h>

class ProjectManagerHolder;

class VectorProjectDefs {
 public:
  typedef VectorProjectEditRequest  Request;
  typedef ProjectManagerHolder       Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_VectorProject;
  static SubmitFuncType        kSubmitFunc;
};


class VectorProject : public AssetDerived<VectorProjectDefs,VectorProject>,
                       public VectorProjectDefs {
 public:
  VectorProject(QWidget* parent);
  VectorProject(QWidget* parent, const Request& req);

  void FinalPreInit(void);
};

#endif  // !KHSRC_FUSION_FUSIONUI_VECTORPROJECT_H__
