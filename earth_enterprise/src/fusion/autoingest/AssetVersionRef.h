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

/******************************************************************************
File:        AssetVersionRef.h
Description: Helper class for extracting/manipulating pieces of a verion ref

-------------------------------------------------------------------------------
******************************************************************************/
#ifndef __AssetVersionRef_h
#define __AssetVersionRef_h

#include <string>
#include <common/khTypes.h>
#include "common/SharedString.h"

class AssetVersionRef {
  std::string assetRef;
  std::string version;
 public:
  AssetVersionRef(const std::string &ref_);
  AssetVersionRef(const SharedString &ref_);
  AssetVersionRef& operator=(const std::string &ref) {
    return (*this = AssetVersionRef(ref));
  }
  operator std::string() const;

  AssetVersionRef(const std::string &assetRef_, const std::string &version_);
  AssetVersionRef(const std::string &assetRef_, unsigned int vernum);

  std::string Bind(void) const;
  static std::string Bind(const AssetVersionRef &o) { return o.Bind(); }

  std::string AssetRef(void) const { return assetRef; }
  std::string Version(void)  const { return version; }
};

#endif /* __AssetVersionRef_h */
