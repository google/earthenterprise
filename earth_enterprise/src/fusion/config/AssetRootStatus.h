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


#ifndef FUSION_CONFIG_ASSETROOTSTATUS_H__
#define FUSION_CONFIG_ASSETROOTSTATUS_H__

#include <string>

// ***** calculates the detailed status of an asset root *****
// All checks are performed with full DAC permissions
// This object should be contructed before calling
// AssetDefs::OverrideAssetRoot().
class AssetRootStatus {
 public:
  AssetRootStatus(const std::string &dirname,
                  const std::string &thishost,
                  const std::string &username,
                  const std::string &groupname);

  std::string  assetroot_;
  std::string  thishost_;
  bool         dir_exists_;
  bool         has_volumes_;
  std::string  master_host_;
  bool         master_active_;
  std::string  version_;
  bool         unique_ids_ok_;
  bool         owner_ok_;


  bool IsMaster(const std::string &thishost) const;
  inline bool IsThisMachineMaster(void) const {
    return IsMaster(thishost_);
  }

  // various higher level tests
  bool AssetRootNeedsUpgrade(void) const;
  bool SoftwareNeedsUpgrade(void) const;
  bool AssetRootNeedsRepair(void) const;


  // throws appropriately worded error message
  void ThrowAssetRootNeedsRepair(void) const;
  void ThrowAssetRootNeedsUpgrade(void) const;
  void ThrowSoftwareNeedsUpgrade(void) const;


  void ValidateForDaemonCheck(void) const;

 private:
  std::string FromHostString(void) const;
};



#endif // FUSION_CONFIG_ASSETROOTSTATUS_H__
