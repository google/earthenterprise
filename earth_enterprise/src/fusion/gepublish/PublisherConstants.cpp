// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// The following constants are used on the server side and the client side
// and are also defined in the java servlet code in:
//   com.google.earth.publisher.Constants

#include "fusion/gepublish/PublisherConstants.h"

// Static initialization
const std::string PublisherConstants::kCmd                = "Cmd";
const std::string PublisherConstants::kCmdPing            = "Ping";
const std::string PublisherConstants::kCmdQuery           = "Query";
const std::string PublisherConstants::kCmdAddDb           = "AddDb";
const std::string PublisherConstants::kCmdDeleteDb        = "DeleteDb";
const std::string PublisherConstants::kCmdPublishDb       = "PublishDb";
const std::string PublisherConstants::kCmdSyncDb          = "SyncDb";
const std::string PublisherConstants::kCmdAddVs           = "AddVs";
const std::string PublisherConstants::kCmdDeleteVs        = "DeleteVs";
const std::string PublisherConstants::kCmdDisableVs       = "DisableVs";
const std::string PublisherConstants::kCmdAddPlugin       = "AddPlugin";
const std::string PublisherConstants::kCmdDeletePlugin    = "DeletePlugin";
const std::string PublisherConstants::kCmdGarbageCollect  = "GarbageCollect";
const std::string PublisherConstants::kCmdDecrementCount  = "DecrementCount";
const std::string PublisherConstants::kCmdLocalTransfer   = "LocalTransfer";

// Request Params.
const std::string PublisherConstants::kQueryCmd              = "QueryCmd";
const std::string PublisherConstants::kQueryCmdListDbs       = "ListDbs";
const std::string PublisherConstants::kQueryCmdListVss       = "ListVss";
const std::string PublisherConstants::kQueryCmdListTgs       = "ListTgs";
const std::string PublisherConstants::kQueryCmdDbDetails     = "DbDetails";
const std::string PublisherConstants::kQueryCmdVsDetails     = "VsDetails";
const std::string PublisherConstants::kQueryCmdPublishedDbs  = "PublishedDbs";
const std::string PublisherConstants::kQueryCmdServerPrefix  = "ServerPrefix";
const std::string PublisherConstants::kQueryCmdHostRoot      = "HostRoot";
const std::string PublisherConstants::kQueryCmdServerHost    = "ServerHost";
const std::string PublisherConstants::kQueryCmdAllowSymLinks = "AllowSymLinks";
const std::string PublisherConstants::kDbName                = "DbName";
const std::string PublisherConstants::kDbPrettyName          = "DbPrettyName";
const std::string PublisherConstants::kDbType                = "DbType";
const std::string PublisherConstants::kFilePath              = "FilePath";
const std::string PublisherConstants::kFileSize              = "FileSize";
const std::string PublisherConstants::kVsName                = "VsName";
const std::string PublisherConstants::kVsType                = "VsType";
const std::string PublisherConstants::kVsUrl                 = "VsUrl";
const std::string PublisherConstants::kVsCacheLevel          = "VsCacheLevel";
const std::string PublisherConstants::kPluginName            = "PluginName";
const std::string PublisherConstants::kClassName             = "ClassName";
const std::string PublisherConstants::kSearchUrl             = "SearchUrl";
const std::string PublisherConstants::kSearchVsName          = "SearchVsName";
const std::string PublisherConstants::kDestFilePath          = "DestFilePath";
const std::string PublisherConstants::kPreferCopy            = "PreferCopy";

// Added to list
const std::string PublisherConstants::kHost                  = "Host";

// Response header names.
const std::string PublisherConstants::kHdrStatusCode      = "Gepublish-StatusCode";
const std::string PublisherConstants::kHdrStatusMessage   = "Gepublish-StatusMessage";
const std::string PublisherConstants::kHdrFileName        = "Gepublish-FileName";
const std::string PublisherConstants::kHdrPluginDetails   = "Gepublish-PluginDetails";
const std::string PublisherConstants::kHdrHostName        = "Gepublish-HostName";
const std::string PublisherConstants::kHdrDbName          = "Gepublish-DbName";
const std::string PublisherConstants::kHdrDbPrettyName    = "Gepublish-DbPrettyName";
const std::string PublisherConstants::kHdrTargetPath      = "Gepublish-TargetPath";
const std::string PublisherConstants::kHdrVsType          = "Gepublish-VsType";
const std::string PublisherConstants::kHdrVsName          = "Gepublish-VsName";
const std::string PublisherConstants::kHdrServerPrefix    = "Gepublish-ServerPrefix";
const std::string PublisherConstants::kHdrServerHost      = "Gepublish-ServerHost";
const std::string PublisherConstants::kHdrServerHostFull  = "Gepublish-ServerHostFull";
const std::string PublisherConstants::kHdrAllowSymLinks   = "Gepublish-AllowSymLinks";
const std::string PublisherConstants::kHdrVsUrl           = "Gepublish-VsUrl";
const std::string PublisherConstants::kHdrDbId            = "Gepublish-DbId";
const std::string PublisherConstants::kHdrHostRoot        = "Gepublish-HostRoot";
const std::string PublisherConstants::kHdrDeleteCount     = "Gepublish-DeleteCount";
const std::string PublisherConstants::kHdrDeleteSize      = "Gepublish-DeleteSize";
const std::string PublisherConstants::kHdrSearchUrl       = "Gepublish-SearchUrl";
const std::string PublisherConstants::kHdrPluginName      = "Gepublish-PluginName";
const std::string PublisherConstants::kHdrPluginClassName = "Gepublish-PluginClassName";
const std::string PublisherConstants::kHdrData            = "Gepublish-Data";

// Response status codes.
const int PublisherConstants::kStatusFailure         = -1;
const int PublisherConstants::kStatusSuccess         = 0;
const int PublisherConstants::kStatusUploadNeeded    = 1;
