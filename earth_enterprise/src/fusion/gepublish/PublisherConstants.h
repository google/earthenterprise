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


#include <string>

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHERCONSTANTS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHERCONSTANTS_H_

namespace PublisherConstants {
  // Constants from the Publisher Servlets com.google.earth.publisher.Constants
  // Request commands.
  extern const std::string kCmd;
  extern const std::string kCmdPing;
  extern const std::string kCmdQuery;
  extern const std::string kCmdAddDb;
  extern const std::string kCmdDeleteDb;
  extern const std::string kCmdPublishDb;
  extern const std::string kCmdSyncDb;
  extern const std::string kCmdAddVs;
  extern const std::string kCmdDeleteVs;
  extern const std::string kCmdDisableVs;
  extern const std::string kCmdAddPlugin;
  extern const std::string kCmdDeletePlugin;
  extern const std::string kCmdGarbageCollect;
  extern const std::string kCmdDecrementCount;
  extern const std::string kCmdLocalTransfer;

  // Request Params.
  extern const std::string kQueryCmd;
  extern const std::string kQueryCmdListDbs;
  extern const std::string kQueryCmdListVss;
  extern const std::string kQueryCmdListTgs;
  extern const std::string kQueryCmdDbDetails;
  extern const std::string kQueryCmdVsDetails;
  extern const std::string kQueryCmdPublishedDbs;
  extern const std::string kQueryCmdServerPrefix;
  extern const std::string kQueryCmdHostRoot;
  extern const std::string kQueryCmdServerHost;
  extern const std::string kQueryCmdAllowSymLinks;
  extern const std::string kDbName;
  extern const std::string kDbPrettyName;
  extern const std::string kDbType;
  extern const std::string kFilePath;
  extern const std::string kFileSize;
  extern const std::string kVsName;
  extern const std::string kVsType;
  extern const std::string kVsUrl;
  extern const std::string kVsCacheLevel;
  extern const std::string kPluginName;
  extern const std::string kClassName;
  extern const std::string kSearchUrl;
  extern const std::string kSearchVsName;
  extern const std::string kDestFilePath;
  extern const std::string kPreferCopy;
  // Added to list (i.e., not in the java list of extern constants)
  extern const std::string kHost;

  // Response header names.
  extern const std::string kHdrStatusCode;
  extern const std::string kHdrStatusMessage;
  extern const std::string kHdrFileName;
  extern const std::string kHdrPluginDetails;
  extern const std::string kHdrHostName;
  extern const std::string kHdrDbName;
  extern const std::string kHdrDbPrettyName;
  extern const std::string kHdrTargetPath;
  extern const std::string kHdrVsType;
  extern const std::string kHdrVsName;
  extern const std::string kHdrServerPrefix;
  extern const std::string kHdrServerHost;
  extern const std::string kHdrServerHostFull;
  extern const std::string kHdrAllowSymLinks;
  extern const std::string kHdrVsUrl;
  extern const std::string kHdrDbId;
  extern const std::string kHdrHostRoot;
  extern const std::string kHdrDeleteCount;
  extern const std::string kHdrDeleteSize;
  extern const std::string kHdrSearchUrl;
  extern const std::string kHdrPluginName;
  extern const std::string kHdrPluginClassName;
  extern const std::string kHdrData;

  // Response status codes.
  extern const int kStatusFailure;
  extern const int kStatusSuccess;
  extern const int kStatusUploadNeeded;
}  // namespace PublisherConstants

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GEPUBLISH_PUBLISHERCONSTANTS_H_
