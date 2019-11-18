#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Publisher constants: request commands and parameters, response header names.
"""

# Delimiter for multi-part parameters (array-parameters).
MULTI_PART_PARAMETER_DELIMITER = ","

POI_SEARCH_SERVICE_NAME = "POISearch"
DEFAULT_SEARCH_SERVICE_NAME = "GeocodingFederated"

HOST_NAME = "Host"

# Request commands.
CMD = "Cmd"
CMD_PING = "Ping"
CMD_RESET = "Reset"
CMD_QUERY = "Query"
CMD_ADD_DB = "AddDb"
CMD_DELETE_DB = "DeleteDb"
CMD_CLEANUP_DB = "CleanupDb"
CMD_PUBLISH_DB = "PublishDb"
CMD_REPUBLISH_DB = "RepublishDb"
CMD_SWAP_TARGETS = "SwapTargets"
CMD_UNPUBLISH_DB = "UnPublishDb"
CMD_SYNC_DB = "SyncDb"
CMD_ADD_VS = "AddVs"
CMD_DELETE_VS = "DeleteVs"
CMD_DISABLE_VS = "DisableVs"
CMD_ADD_SEARCH_DEF = "AddSearchDef"
CMD_DELETE_SEARCH_DEF = "DeleteSearchDef"
CMD_ADD_PLUGIN = "AddPlugin"
CMD_DELETE_PLUGIN = "DeletePlugin"
CMD_DECREMENT_COUNT = "DecrementCount"
CMD_LOCAL_TRANSFER = "LocalTransfer"
CMD_GARBAGE_COLLECT = "GarbageCollect"
CMD_CLEANUP = "Cleanup"
CMD_ADD_SNIPPET_SET = "AddSnippetSet"
CMD_DELETE_SNIPPET_SET = "DeleteSnippetSet"

# Request Params.
QUERY_CMD = "QueryCmd"
QUERY_CMD_LIST_DBS = "ListDbs"
QUERY_CMD_LIST_ASSETS = "ListAllAssets"
QUERY_CMD_DB_DETAILS = "DbDetails"
QUERY_CMD_LIST_VSS = "ListVss"
QUERY_CMD_VS_DETAILS = "VsDetails"
QUERY_CMD_LIST_TGS = "ListTgs"
QUERY_CMD_TARGET_DETAILS = "TargetDetails"
QUERY_CMD_LIST_SEARCH_DEFS = "ListSearchDefs"
QUERY_CMD_SEARCH_DEF_DETAILS = "SearchDefDetails"
QUERY_CMD_LIST_SNIPPET_SETS = "ListSnippetSets"
QUERY_CMD_SNIPPET_SET_DETAILS = "SnippetSetDetails"
QUERY_CMD_LIST_META_FIELD_PATHS = "ListMetaFieldPaths"
QUERY_CMD_META_FIELDS_SET = "MetaFieldsSet"
QUERY_CMD_PUBLISHED_DB_DETAILS = "PublishedDbDetails"
QUERY_CMD_PUBLISHED_DBS = "PublishedDbs"
QUERY_CMD_SERVER_PREFIX = "ServerPrefix"
QUERY_CMD_HOST_ROOT = "HostRoot"
QUERY_CMD_SERVER_HOST = "ServerHost"
QUERY_CMD_ALLOW_SYM_LINKS = "AllowSymLinks"
QUERY_CMD_LIST_PLUGIND = "ListPlugins"
QUERY_CMD_GEDB_PATH = "GeDbPath"
DB_ID = "DbId"
DB_NAME = "DbName"
DB_PRETTY_NAME = "DbPrettyName"
DB_TYPE = "DbType"
DB_TIMESTAMP = "DbTimestamp"
DB_SIZE = "DbSize"
DB_USE_GOOGLE_BASEMAP = "DbUseGoogleBasemap"
FILE_PATH = "FilePath"
FILE_SIZE = "FileSize"
VS_NAME = "VsName"
VS_TYPE = "VsType"
VS_URL = "VsUrl"
VS_SSL = "VsSsl"
VS_CACHE_LEVEL = "VsCacheLevel"
PLUGIN_NAME = "PluginName"
CLASS_NAME = "ClassName"
SEARCH_URL = "SearchUrl"
SEARCH_VS_NAME = "SearchVsName"
DEST_FILE_PATH = "DestFilePath"
FORCE_COPY = "ForceCopy"
PREFER_COPY = "PreferCopy"
TARGET_PATH = "TargetPath"
TARGET_PATH_A = "TargetPathA"
TARGET_PATH_B = "TargetPathB"
VIRTUAL_HOST_NAME = "VirtualHostName"
SEARCH_DEF_NAME = "SearchDefName"
SUPPLEMENTAL_SEARCH_DEF_NAME = "SupSearchDefName"
SEARCH_DEF = "SearchDef"
POI_FEDERATED = "PoiFederated"
POI_SUGGESTION = "PoiSuggestion"
NEED_SEARCH_TAB_ID = "NeedSearchTabId"
SUPPLEMENTAL_UI_LABEL = "SupUiLabel"
SNIPPET_SET_NAME = "SnippetSetName"
SNIPPET_SET = "SnippetSet"
SERVE_WMS = "ServeWms"
EC_DEFAULT_DB = "EcDefaultDb"
ORIGIN_REQUEST_HOST = "OriginRequestHost"

# Response header names.
HDR_STATUS_CODE = "Gepublish-StatusCode"
HDR_STATUS_MESSAGE = "Gepublish-StatusMessage"
HDR_FILE_NAME = "Gepublish-FileName"
HDR_PLUGIN_DETAILS = "Gepublish-PluginDetails"
HDR_HOST_NAME = "Gepublish-HostName"
HDR_DB_NAME = "Gepublish-DbName"
HDR_DB_PRETTY_NAME = "Gepublish-DbPrettyName"
HDR_TARGET_PATH = "Gepublish-TargetPath"
HDR_VS_TYPE = "Gepublish-VsType"
HDR_VS_NAME = "Gepublish-VsName"
HDR_SERVER_PREFIX = "Gepublish-ServerPrefix"
HDR_SERVER_HOST = "Gepublish-ServerHost"
HDR_SERVER_HOST_FULL = "Gepublish-ServerHostFull"
HDR_ALLOW_SYM_LINKS = "Gepublish-AllowSymLinks"
HDR_VS_URL = "Gepublish-VsUrl"
HDR_DB_ID = "Gepublish-DbId"
HDR_HOST_ROOT = "Gepublish-HostRoot"
HDR_DELETE_COUNT = "Gepublish-DeleteCount"
HDR_DELETE_SIZE = "Gepublish-DeleteSize"
HDR_SEARCH_URL = "Gepublish-SearchUrl"
HDR_PLUGIN_NAME = "Gepublish-PluginName"
HDR_PLUGIN_CLASS_NAME = "Gepublish-PluginClassName"
HDR_DATA = "Gepublish-Data"

HDR_JSON_RESULTS = "results"
HDR_JSON_STATUS_CODE = "status_code"
HDR_JSON_STATUS_MESSAGE = "status_message"

# TODO: Get from mod_fdb!?
CUTTER_GLOBES_PATH = "/opt/google/gehttpd/htdocs/cutter/globes"

# Response status codes.
STATUS_FAILURE = -1
STATUS_SUCCESS = 0
STATUS_UPLOAD_NEEDED = 1
