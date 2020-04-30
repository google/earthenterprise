/*
 * Copyright 2020 The Open GEE Contributors
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

#ifndef ASSETOPERATION_H
#define ASSETOPERATION_H

#include "common/SharedString.h"
#include "fusion/autoingest/sysman/.idl/TaskStorage.h"
#include "MiscConfig.h"

void RebuildVersion(const SharedString & ref, MiscConfig::GraphOpsType graphOps = MiscConfig::Instance().GraphOperations);
void CancelVersion(const SharedString & ref, MiscConfig::GraphOpsType graphOps = MiscConfig::Instance().GraphOperations);
void HandleTaskProgress(const TaskProgressMsg & msg, MiscConfig::GraphOpsType graphOps = MiscConfig::Instance().GraphOperations);
void HandleTaskDone(const TaskDoneMsg & msg, MiscConfig::GraphOpsType graphOps = MiscConfig::Instance().GraphOperations);
void HandleExternalStateChange(
  const SharedString & ref,
  AssetDefs::State oldState,
  std::uint32_t numInputsWaitingFor,
  std::uint32_t numChildrenWaitingFor,
  MiscConfig::GraphOpsType graphOps = MiscConfig::Instance().GraphOperations);

#endif // ASSETOPERATION_H
