/*
 * Copyright 2017 Google Inc.
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

#ifndef KHSRC_FUSION_GST_GSTVECTORPROJECT_H__
#define KHSRC_FUSION_GST_GSTVECTORPROJECT_H__

#include <vector>

#include <gstMemory.h>
#include <autoingest/.idl/storage/VectorProjectConfig.h>

//
// Styles can never be 0.  This has special meaning to the server.
//
// Channels must never be 0-4 as the client has
// legacy code that has special-cased these channels
//   ( original use: 1=roads, 2=terrain, 3=roads, 4=weather )
//
#define StandardChannelStart    5
#define StandardStyleStart      1
#define OverlayChannelStart 10000
#define OverlayStyleStart  100000

class gstSource;
class gstLayer;

class gstVectorProject {
  friend class ProjectManager;
 public:
  gstVectorProject(const std::string& name);
  ~gstVectorProject();

  bool Prefill(const VectorProjectConfig& cfg);
  void AssembleEditRequest(VectorProjectEditRequest *request);
  bool HasSearchFields(void) const;

  VectorProjectConfig::ServerType serverType() const {
    return config_.serverType;
  }
  void serverType(VectorProjectConfig::ServerType type);

  unsigned int buildVersion() const { return config_.indexVersion; }
  void buildVersion(unsigned int v);

 private:
  std::string name_;
  VectorProjectConfig config_;
  std::vector<gstLayer*> layers_;
};

#endif  // !KHSRC_FUSION_GST_GSTVECTORPROJECT_H__
