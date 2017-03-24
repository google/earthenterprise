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

#ifndef __khVolumeManager_h
#define __khVolumeManager_h

#include <autoingest/.idl/VolumeStorage.h>
#include "khFusionURI.h"


class khVolumeManager : private VolumeDefList
{
 protected:
  static std::string VolumeListFilename(void);

 public:
  khVolumeManager(void);
  void Init(void);

  std::string NetworkPathOf(const khFusionURI &) const;
  std::string LocalPathOf(const khFusionURI &) const;
  khFusionURI DeduceURIFromPath(const std::string &path) const;

  void GetHostVolumes(const std::string &host,
                      std::vector<std::string> &volnames) const;
  void GetLocalTmpVolumes(const std::string &host,
                          std::vector<std::string> &volnames) const;
  void GetRemoteTmpVolumes(const std::string &host,
                           std::vector<std::string> &volnames) const;
  std::string VolumeHost(const std::string &volname) const;
  const VolumeDef* GetVolumeDef(const std::string &volname) const;
};

// different storage in sysman than in read only autoingest lib
// see sysman/khResourceManager.cpp and ReadOnlyExtra.cpp
extern khVolumeManager& theVolumeManager;

#endif /* __khVolumeManager_h */
