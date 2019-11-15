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

"""Publish manager module.

Classes for managing publish serving (high level logic of request handling).
"""

import json
import logging
import os
import time
import urlparse


from common import exceptions
from common import utils

import libgepublishmanagerhelper

from serve import basic_types
from serve import constants
from serve import http_io
from serve import serve_utils
from serve import snippets_db_manager
from serve.mod_fdb_serve import mod_fdb_serve_handler
from serve.publish import publish_manager_helper
from serve.snippets.util import dbroot_writer


logger = logging.getLogger("ge_stream_publisher")

RESERVED_WORD_SET = ["fdb", "htdocs", "admin", "cutter", "earth", "icons",
                     "js", "maps", "portable", "shared_assets"]


class PublishDef(object):
  """Class encapsulating set of publish parameters."""

  def __init__(self):
    # The target path, publish point.
    self.target_path = None
    # A name of the virtual host used for publishing.
    self.virtual_host_name = None
    # A database name (an assetroot path).
    self.db_name = None
    # A hostname of the Fusion-client.
    self.client_host_name = None
    # A database type.
    self.db_type = None
    # A name of a snippet set assigned to the publish-point.
    self.snippets_set_name = None
    # A list of search services (list of search definition names) being added
    # to a database.
    self.search_tabs = None
    # A list of supplemental search services (list of search definition names)
    # being added to a database.
    self.sup_search_tabs = None
    # A text to appear under POI search box as a suggestion.
    self.poi_suggestion = None
    # Whether to append an ID (target path) to a Search Tab label.
    self.need_search_tab_id = False
    # Whether the POISearch is combined with the GeocodingFederated search.
    self.poi_federated = False
    # A supplemental UI label.
    self.supplemental_ui_label = None
    # Whether the target point shall be servable through WMS-service.
    self.serve_wms = False
    # Whether to force copy publish-manifest files.
    self.force_copy = False
    # Whether to make this databased the default for Earth Clients when
    # none is specified in stream endpoint requests. 
    self.ec_default_db = False


# TODO: rename to PublishHandler.
class PublishManager(object):
  """Class for managing publish serving."""

  # TODO: decide whether these will be read from some config?
  VS_CONFIG_PATH = "/opt/google/gehttpd/conf.d/virtual_servers"

  HTDOCS_EARTH_PATH = "/opt/google/gehttpd/htdocs/earth"
  SUPPLEMENTAL_SEARCH_UI_HTML = "search_supplemental_ui.html"

  SUPPLEMENTAL_UI_LABEL_DEFAULT = "More..."
  # Use relative path for supplemental search URL.
  # The 'search_html' is a command managed by fdb-module.
  SUPPLEMENTAL_SEARCH_URL = "search_html"

  SEARCH_HTML = "search.html"
  SEARCH_JSON_PATH = "json/search.json"

  def __init__(self):
    """Inits publish manager."""
    self._publish_helper = publish_manager_helper.PublishManagerHelper()
    self._snippets_manager = snippets_db_manager.SnippetsDbManager()
    self._mod_fdb_serve_handler = mod_fdb_serve_handler.ModFdbServeHandler()
    server_url = utils.BuildServerUrl("localhost")
    if not server_url:
      raise exceptions.PublishServeException(
          "Couldn't get self-referential server URL.")
    logger.debug("PublishManager init: server URL %s", server_url)
    self._server_url = server_url
    self.__ResetPublishManager(server_url)

  def HandlePingRequest(self, request, response):
    """Handles ping server request.

    Args:
      request: request object.
      response: response object.
    """
    self._publish_helper.HandlePingRequest(request, response)

  def HandleCleanupRequest(self, request, response):
    """Handles publisher cleanup request.

    Cleans up publish info tables.
    Returns in response a list of unpublished Databases [{host:, path:}, ..].
    Args:
      request: request object.
      response: response object.
    """
    cmd = request.GetParameter(constants.CMD)
    assert cmd and (cmd == constants.CMD_CLEANUP)

    unpublished_dbs = self._publish_helper.Cleanup()
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_DATA, json.dumps(unpublished_dbs))
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def HandleResetRequest(self, request, response):
    """Handles publisher reset request.

    Cleans up publish info tables, recreates Readers/Unpackers for published
    databases and updates publish info in .htacces file.

    Args:
      request: request object.
      response: response object.
    """
    logger.debug("HandleResetRequest...")
    self.__ResetPublishManager(self._server_url)
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def HandleQueryRequest(self, request, response):
    """Handles query requests.

    Args:
      request: request object.
      response: response object.
    """
    logger.debug("HandleQueryRequest...")
    self._publish_helper.HandleQueryRequest(request, response)

  def HandlePublishRequest(self, request, response):
    """Handles publish database request.

    Args:
      request: request object.
      response: response object.
    Raises:
      PublishServeException, psycopg2.Error/Warning.
    """
    logger.debug("HandlePublishRequest...")
    publish_def = self.__GetPublishParameters(request)

    db_id = 0

    # Handling Fusion database.
    if serve_utils.IsFusionDb(publish_def.db_type):
      # Check if the DB exists.
      db_id = self._publish_helper.QueryDbId(publish_def.client_host_name,
                                             publish_def.db_name)
      if db_id == 0:
        raise exceptions.PublishServeException(
            "Database %s does not exist on server.\n"
            "It needs to be registered/pushed before publishing." %
            publish_def.db_name)

      gedb_path = self._publish_helper.BuildDbPublishPath(
          publish_def.client_host_name, publish_def.db_name)
      logger.debug("GEDB path: %s, db type: %s", gedb_path, publish_def.db_type)

      target_gedb_path = self._publish_helper.BuildTargetPublishPath(
          gedb_path, publish_def.target_path)

      self.__GetPublishManifest(publish_def)
    # Handling portable globe.
    elif serve_utils.IsPortable(publish_def.db_type):
      # Handling portable globes.
      # Note: client host name is not used for portable globes.
      assert (isinstance(publish_def.client_host_name, str) and
              (not publish_def.client_host_name))
      target_gedb_path = publish_def.db_name
      db_id = self._publish_helper.QueryDbId(publish_def.client_host_name,
                                             publish_def.db_name)
      if db_id == 0:
        raise exceptions.PublishServeException(
            "Database %s does not exist (not registered/not pushed)." %
            publish_def.db_name)
    else:
      raise exceptions.PublishServeException("Unsupported DB type %s." %
                                             publish_def.db_type)

    assert db_id != 0

    try:
      # Register FusionDb/Portable for serving in Fdb module.
      self._mod_fdb_serve_handler.RegisterDatabaseForServing(
          self._server_url,
          publish_def.target_path,
          publish_def.db_type,
          target_gedb_path)
    except Exception:
      self._publish_helper.DeleteTargetPublishDir(
          publish_def.target_path,
          publish_def.client_host_name,
          publish_def.db_name)
      raise

    try:
      # Add entry to target_db_table.
      self._publish_helper.HandlePublishRequest(db_id, publish_def)

      # Update .htaccess file.
      self._publish_helper.UpdateHtaccessFile()
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_CODE,
                                            constants.STATUS_SUCCESS)
    except Exception:
      self._publish_helper.DoUnpublish(publish_def.target_path)
      self._mod_fdb_serve_handler.UnregisterDatabaseForServing(
          self._server_url, publish_def.target_path)
      raise

  def HandleUnpublishRequest(self, request, response):
    """Handles un-publish request.

    Cleans up an entry in target_db_table, updates .htaccess file, unregister
    database for serving in Fdb module (clears corresponding Reader/Unpacker),
    deletes the publish manifest files.

    Args:
      request: request object.
      response: response object.
    Raises:
      psycopg2.Error/Warning, PublishServeException.
    """
    # Get target path parameter.
    target_path = request.GetParameter(constants.TARGET_PATH)
    if not target_path:
      raise exceptions.PublishServeException(
          "HandleUnpublishRequest: Missing target path.")

    assert isinstance(target_path, str)

    norm_target_path = serve_utils.NormalizeTargetPath(target_path)
    if not norm_target_path:
      raise exceptions.PublishServeException(
          "Not valid target path %s (path format is /sub_path1[/sub_path2]." %
          target_path)


    try:
      self._publish_helper.HandleUnpublishRequest(norm_target_path)
    except Exception:
      # Unregister FusionDb/Portable for serving in Fdb module.
      self._mod_fdb_serve_handler.UnregisterDatabaseForServing(
          self._server_url, norm_target_path)
      raise
    else:
      # Unregister FusionDb/Portable for serving in Fdb module.
      self._mod_fdb_serve_handler.UnregisterDatabaseForServing(
          self._server_url, norm_target_path)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_CODE,
                                            constants.STATUS_SUCCESS)

  def HandleAddVsRequest(self, request, response):
    """Handles add virtual server request.

    Args:
      request: request object.
      response: response object.
    Raises:
      psycopg2.Error/Warning, PublishServeException.
    """
    default_vs_cache_level = "2"
    vs_name = request.GetParameter(constants.VS_NAME)
    vs_path = request.GetParameter(constants.VS_URL)
    vs_ssl = request.GetParameter(constants.VS_SSL)
    vs_cache_level = request.GetParameter(constants.VS_CACHE_LEVEL)
    if not vs_name:
      raise exceptions.PublishServeException("Missing Virtual Host name.")

    if not vs_path:
      vs_path = "/%s" % vs_name

    if not vs_cache_level:
      vs_cache_level = default_vs_cache_level

    self._publish_helper.HandleAddVsRequest(
        vs_name, vs_path, bool(vs_ssl), vs_cache_level, response)

  def HandleDeleteVsRequest(self, request, response):
    """Handles delete virtual server request.

    Args:
      request: request object.
      response: response object.
    Raises:
     psycopg2.Error/Warning, PublishServeException.
    """
    vs_name = request.GetParameter(constants.VS_NAME)
    if not vs_name:
      raise exceptions.StreamPublisherServletException(
          "HandleDeleteVsRequest: Missing Virtual server name.")

    self._publish_helper.HandleDeleteVsRequest(vs_name, response)

  def __ResetPublishManager(self, server_url):
    """Resets publish manager.

    Resets FdbService (unregisters FusionDbs/Portables registered for serving,
    clears all existing Readers/Unpackers),
    Cleans up publish info tables,
    Sets cut specification into Fdb module,
    Re-registers for serving published FusionDbs/Portables in Fdb
    module,
    Updates publish info in .htacccess-file.

    Args:
      server_url: server URL (scheme://host:port).
    Raises:
      psycopg2.Error/Warning, PublishServeException, ModFdbServeException.
    """
    # Clear all Readers/Unpackers.
    self._mod_fdb_serve_handler.ResetFdbService(server_url)

    # Init cut specs.
    self.__InitCutSpecs(server_url)

    # Register all published targets for serving in Fdb module.
    published_list = self._publish_helper.GetPublishInfoList()
    for (target_path, host_name, db_name) in published_list:
      (norm_db_name, db_type) = serve_utils.IdentifyPublishedDb(db_name)
      if serve_utils.IsFusionDb(db_type):
        gedb_path = self._publish_helper.BuildDbPublishPath(
            host_name, norm_db_name)
        target_gedb_path = self._publish_helper.BuildTargetPublishPath(
            gedb_path, target_path)
      else:
        assert serve_utils.IsPortable(db_type)
        target_gedb_path = norm_db_name

      try:
        self._mod_fdb_serve_handler.RegisterDatabaseForServing(
            server_url, target_path, db_type, target_gedb_path)
      except exceptions.PublishServeException as e:
        # Unpublish the target path if registering for serving has failed.
        logger.error(e)
        self._publish_helper.DoUnpublish(target_path)

    # Update .htaccess file.
    self._publish_helper.UpdateHtaccessFile()

  def __InitCutSpecs(self, server_url):
    """Initializes cut specs to allow dynamic cutting.

    Sends a request to instantiate each cut spec that has been
    defined in the database. The cut specs allow dynamic cutting of a database,
    limiting the extent of a database that is served on a given mount point.
    Args:
      server_url: server URL (scheme://host:port).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    cut_spec_list = self._publish_helper.GetCutSpecs()
    if cut_spec_list:
      self._mod_fdb_serve_handler.InitCutSpecs(server_url, cut_spec_list)

  def __GetPublishParameters(self, request):
    """Gets and verifies all publish request parameters.

    Args:
      request: request object.
    Returns:
      The PublishDef object encapsulating set of the publish parameters.
    """
    publish_def = PublishDef()

    # Get Virtual Host name parameter.
    publish_def.virtual_host_name = request.GetParameter(
        constants.VIRTUAL_HOST_NAME)
    if not publish_def.virtual_host_name:
      raise exceptions.PublishServeException("Missing Virtual Host name.")
    assert isinstance(publish_def.virtual_host_name, str)

    # Get target path
    target_path = request.GetParameter(constants.TARGET_PATH)
    if not target_path:
      raise exceptions.PublishServeException("Missing target path.")
    assert isinstance(target_path, str)

    # Normalize target path.
    publish_def.target_path = serve_utils.NormalizeTargetPath(target_path)
    if not publish_def.target_path:
      raise exceptions.PublishServeException(
          "Not valid target path %s (path format is /sub_path1[/sub_path2])." %
          target_path)

    self.__VerifyTargetPath(publish_def.target_path)
    logger.debug("target path: %s", publish_def.target_path)

    # Check whether target path is already in use.
    if self._publish_helper.IsTargetPathUsed(publish_def.target_path):
      raise exceptions.PublishServeException(
          "Target path %s is already in use. Note that paths are "
          "case insensitve. Input another path or"
          " un-publish database using this path." % publish_def.target_path)

    # Get db name parameter.
    publish_def.db_name = request.GetParameter(constants.DB_NAME)
    if not publish_def.db_name:
      raise exceptions.PublishServeException(
          "Missing database name.")
    logger.debug("db_name: %s", publish_def.db_name)
    assert isinstance(publish_def.db_name, str)

    (publish_def.db_name,
     publish_def.db_type) = serve_utils.IdentifyPublishedDb(
         publish_def.db_name)

    if serve_utils.IsFusionDb(publish_def.db_type):
      # Get host name parameter that is part of database ID (host, db_name).
      publish_def.client_host_name = request.GetParameter(
          constants.HOST_NAME)
      if not publish_def.client_host_name:
        raise exceptions.PublishServeException("Missing Hostname.")

      assert isinstance(publish_def.client_host_name, str)
    elif serve_utils.IsPortable(publish_def.db_type):
      # There is no a client hostname for Portable databases.
      publish_def.client_host_name = ""
    else:
      raise exceptions.PublishServeException("Unsupported DB type %s." %
                                             publish_def.db_type)

    # Get snippets set name parameter.
    publish_def.snippets_set_name = request.GetParameter(
        constants.SNIPPET_SET_NAME)
    logger.debug("Snippets set name: %s ", publish_def.snippets_set_name if
                 publish_def.snippets_set_name else "not specified.")

    # Get search definition name parameter.
    publish_def.search_tabs = request.GetMultiPartParameter(
        constants.SEARCH_DEF_NAME)
    if publish_def.search_tabs:
      logger.debug("Search tabs: %s", publish_def.search_tabs)
    else:
      logger.debug("Search tabs are not specified.")

    # Get supplemental search definition name parameter.
    publish_def.sup_search_tabs = request.GetMultiPartParameter(
        constants.SUPPLEMENTAL_SEARCH_DEF_NAME)
    if publish_def.sup_search_tabs:
      logger.debug("Supplemental search tabs: %s",
                   publish_def.sup_search_tabs)
    else:
      logger.debug("Supplemental search tabs are not specified.")

    publish_def.need_search_tab_id = request.GetBoolParameter(
        constants.NEED_SEARCH_TAB_ID)

    publish_def.poi_federated = request.GetBoolParameter(
        constants.POI_FEDERATED)

    publish_def.poi_suggestion = request.GetParameter(
        constants.POI_SUGGESTION)

    publish_def.supplemental_ui_label = request.GetParameter(
        constants.SUPPLEMENTAL_UI_LABEL)

    publish_def.serve_wms = request.GetBoolParameter(constants.SERVE_WMS)

    publish_def.force_copy = request.IsForceCopy()

    publish_def.ec_default_db = request.GetBoolParameter(constants.EC_DEFAULT_DB)
    
    return publish_def

  def __GetPublishManifest(self, publish_def):
    """Build publish manifest files.

    Publish manifest files: dbroot.v5p.DEFAULT, {earth,maps}.json, search.json,
    serverdb.config, search.manifest.
    Args:
      publish_def: a PublishDef object encapsulating set of the publish
          parameters.
    """
    assert serve_utils.IsFusionDb(publish_def.db_type)

    # Get publish URLs.
    # Build stream URL based on Virtual Host URL.
    vh_url, vh_ssl = self._publish_helper.QueryVh(publish_def.virtual_host_name)
    vh_base_url = self._publish_helper.GetVhBaseUrl(vh_url, vh_ssl)
    stream_url = urlparse.urljoin(vh_base_url, publish_def.target_path)

    logger.debug("Stream URL: %s", stream_url)

    # Get database ID from gesearch database.
    search_db_id = self._publish_helper.GetSearchDbId(
        publish_def.client_host_name, publish_def.db_name)

    # Build end_snippet proto section.
    # Set end_snippet_proto to empty string - nothing to merge.
    snippets_set = None
    end_snippet_proto = None
    if publish_def.snippets_set_name:
      snippets_set = self._snippets_manager.GetSnippetSetDetails(
          publish_def.snippets_set_name)
      logger.debug("Snippets set: %s", snippets_set)

    # Get list of search definitions.
    search_def_list = self.__GetSearchDefs(
        publish_def.search_tabs,
        False,  # is_supplemental
        search_db_id,
        publish_def.poi_federated,
        publish_def.poi_suggestion)

    # Get list of supplemental search definitions.
    sup_search_def_list = self.__GetSearchDefs(
        publish_def.sup_search_tabs,
        True,  # is_supplemental
        search_db_id,
        publish_def.poi_federated,
        publish_def.poi_suggestion)

    supplemental_ui_label = None
    if sup_search_def_list:
      if len(sup_search_def_list) == 1:
        supplemental_ui_label = sup_search_def_list[0].label
      else:
        supplemental_ui_label = PublishManager.SUPPLEMENTAL_UI_LABEL_DEFAULT
      if publish_def.supplemental_ui_label:
        # Override with user specified value for the supplemental UI label.
        supplemental_ui_label = publish_def.supplemental_ui_label

    if publish_def.db_type == basic_types.DbType.TYPE_GE:
      # Get end_snippet of protobuf dbroot - integrates snippets set and search
      # services.
      search_tab_id = None
      if publish_def.need_search_tab_id:
        # Using a target path as an ID for main search services by appending it
        # to a search service label allows us to differentiate search tabs from
        # different databases in Earth Client.
        search_tab_id = publish_def.target_path[1:]

      supplemental_search_url = PublishManager.SUPPLEMENTAL_SEARCH_URL
      end_snippet_proto = dbroot_writer.CreateEndSnippetProto(
          snippets_set,
          search_def_list,
          search_tab_id,
          supplemental_ui_label,
          supplemental_search_url,
          logger)

      # Note: useful for debugging.
    #      if __debug__:
    #        logger.debug("Proto end_snippet: %s", end_snippet_proto)

    try:
      # Get publish manifest.
      publish_helper = libgepublishmanagerhelper.PublishManagerHelper(logger)
      publish_manifest = libgepublishmanagerhelper.ManifestEntryVector()

      # Prepare publish config.
      publish_config = libgepublishmanagerhelper.PublishConfig()
      publish_config.fusion_host = publish_def.client_host_name
      publish_config.db_path = publish_def.db_name
      publish_config.stream_url = stream_url
      publish_config.end_snippet_proto = (
          end_snippet_proto if end_snippet_proto else "")
      publish_config.server_prefix = self._publish_helper.server_prefix

      publish_helper.GetPublishManifest(publish_config, publish_manifest)

      logger.debug("PublishDatabase: publish manifest size %s.",
                   len(publish_manifest))

      if not publish_manifest:
        raise exceptions.PublishServeException(
            "Unable to create publish manifest. Database is not pushed.")

      # Creates the search.json file and append it to publish manifest.
      # Note: we append the search.json and supplemental search
      # UI html files to the publish manifest if it is needed.
      if sup_search_def_list:
        search_def_list.extend(sup_search_def_list)

      if search_def_list:
        publish_tmp_dir_path = os.path.normpath(publish_helper.TmpDir())
        search_json_local_path = (
            "%s/%s" % (publish_tmp_dir_path, PublishManager.SEARCH_JSON_PATH))
        self. __CreateSearchJsonFile(search_def_list, search_json_local_path)
        search_json_dbroot_path = PublishManager.SEARCH_JSON_PATH

        publish_manifest.push_back(
            libgepublishmanagerhelper.ManifestEntry(search_json_dbroot_path,
                                                    search_json_local_path))

        # Append supplemental search UI html to publish manifest.
        if ((publish_def.db_type == basic_types.DbType.TYPE_GE) and
            supplemental_ui_label):
          search_html_local_path = os.path.join(
              PublishManager.HTDOCS_EARTH_PATH,
              PublishManager.SUPPLEMENTAL_SEARCH_UI_HTML)
          search_html_dbroot_path = PublishManager.SEARCH_HTML
          publish_manifest.push_back(
              libgepublishmanagerhelper.ManifestEntry(search_html_dbroot_path,
                                                      search_html_local_path))

      # {gedb/,mapdb/} path is {server_prefix}/{fusion_host}{db_path}.
      gedb_path = self._publish_helper.BuildDbPublishPath(
          publish_def.client_host_name, publish_def.db_name)

      # Transfer publish manifest files into published DBs directory.
      db_path_prefix = self._publish_helper.BuildTargetPublishPath(
          gedb_path, publish_def.target_path)

      self._TransferPublishManifest(
          publish_manifest, db_path_prefix, publish_def.force_copy)
    except Exception:
      # Delete target's publish directory in case of any error.
      self._publish_helper.DeleteTargetPublishDir(
          publish_def.target_path,
          publish_def.client_host_name,
          publish_def.db_name)
      raise
    finally:
      # Reset PublishManagerHelper processor (deletes publish temp. directory
      # /tmp/publish.*).
      publish_helper.Reset()

  def __GetSearchDefs(self, search_def_name_list, is_supplemental,
                      search_db_id, poi_federated, poi_suggestion):
    """Gets list of search definition objects based on list of names.

    Args:
      search_def_name_list: list of search definition names.
      is_supplemental: whether they are supplemental search services.
      search_db_id: search database ID.
      poi_federated: whether to combine POI search with GeocodingFederated.
      poi_suggestion: text to appear under search box as suggestion.
    Returns:
      list of search definitions.
    """
    search_def_list = []
    if not search_def_name_list:
      return search_def_list

    assert isinstance(search_def_name_list, list)
    logger.debug("Search def name list: %s", search_def_name_list)

    if constants.POI_SEARCH_SERVICE_NAME in search_def_name_list:
      if search_db_id == 0:
        raise exceptions.PublishServeException(
            "Internal Error - "
            "POISearch is specified, while the database has no POI data.")

    for search_def_name in search_def_name_list:
      search_def_json = self._publish_helper.GetSearchDefDetails(
          search_def_name)
      logger.debug("Search def: %s", search_def_json)
      if search_def_json:
        search_def = basic_types.SearchDef.Deserialize(search_def_json)
        if not search_def.service_url:
          raise exceptions.PublishServeException(
              "Internal Error -"
              " service URL is not defined in search definition.")

        # For POI search service set additional parameters.
        if search_def_name == constants.POI_SEARCH_SERVICE_NAME:
          # Set suggestion string.
          if search_def.fields:
            search_def.fields[0].suggestion = poi_suggestion
          # Add POI federated parameter.
          poi_federated_param = (
              "%s=%d" % (constants.POI_FEDERATED, int(poi_federated)))
          search_def.additional_query_param = utils.JoinQueryStrings(
              search_def.additional_query_param,
              poi_federated_param)

        # Set flag whether to show in supplemental UI.
        search_def.supplemental_ui_hidden = not is_supplemental

        # Check whether main search service has one search field.
        if (search_def.supplemental_ui_hidden and
            (len(search_def.fields) > 1)):
          # Search services with multiple search fields can't be assigned on
          # Earth Client search tab. They should go to supplemental UI search,
          # so raise an exception.
          raise exceptions.PublishServeException(
              "Internal Error - search tab with multiple search fields.")

        logger.debug("SearchDef: %s", search_def.DumpJson())
        search_def_list.append(search_def)
      else:
        raise exceptions.PublishServeException(
            "Internal Error - "
            "Could not get search definition for search service: %s." %
            search_def_name)

    return search_def_list

  def __CreateSearchJsonFile(self, search_def_list, search_json_path):
    """Creates the search.json file and serialize list of search definitions.

    Example of search.json file content:
    [
    {"supplemental_ui_hidden": true,
    "additional_config_param": null,
    "label": "Places or Coordinates",
    "additional_query_param":
      "flyToFirstElement=true&displayKeys=location&DbId=1&PoiFederated=1",
    "service_url": "/FederatedSearch",
    "fields": [{"key": "q",
               "suggestion": "City, Country or lat, lng (e.g., 34.67, -172.45)",
               "label": null}]},
    ...]

    Args:
      search_def_list: list of search definition to serialize.
      search_json_path: path to search.json file.
    """
    if not search_def_list:
      return

    assert isinstance(search_def_list, list)

    if not os.path.exists(os.path.dirname(search_json_path)):
      raise exceptions.PublishServeException(
          "Internal error - publish manifest directory does not exist.")

    with open(search_json_path, "w") as fp:
      fp.write(json.dumps(search_def_list,
                          cls=basic_types.SearchDefJsonEncoder))

  def _TransferPublishManifest(self, publish_manifest, db_path_prefix,
                               force_copy):
    """Transfers publish manifest files into published database directory.

    Args:
      publish_manifest: publish manifest.
      db_path_prefix: database path prefix.
      force_copy: whether to force a copy.
    Raises:
      exceptions.PublishServeException
    """
    for item in publish_manifest:
      src_path = item.current_path
      dest_path = "%s/%s" % (db_path_prefix, item.orig_path)
      logger.debug("TransferPublishManifest - src_path: %s, dest_path: %s.",
                   src_path, dest_path)

      # Transfer manifest file to published database directory.
      tries = 2
      sleep_secs = 5
      while (not serve_utils.LocalTransfer(
          src_path, dest_path,
          force_copy, prefer_copy=True, allow_symlinks=False)):
        tries -= 1
        if tries == 0:
          raise exceptions.PublishServeException(
              "Could not transfer publish manifest file %s to %s." %
              (src_path, dest_path))
        logger.debug("Retrying Local Transfer.")
        time.sleep(sleep_secs)
        sleep_secs *= 2      # Double the sleep time after each retry.

  def __VerifyTargetPath(self, t_path):
    """Verifies target path for validity.

    Check if the reserved word is used in the path.
    Args:
      t_path: the path to verify.
    Raises:
      PublishServeException: in case the path is not valid.
    """
    if not t_path:
      return

    assert t_path and (t_path[0] == "/") and (t_path[-1] != "/")
    end_idx = t_path.find("/", 1)
    sub_path = t_path[1:] if (end_idx == -1) else t_path[1:end_idx]
    if sub_path in RESERVED_WORD_SET:
      raise exceptions.PublishServeException(
          "System reserved word %s is used in target path %s." % (sub_path,
                                                                  t_path))

    vh_path_list = self._publish_helper.GetVsUrlPathList()
    sub_path = t_path if end_idx == -1 else t_path[:end_idx]
    if sub_path in vh_path_list:
      raise exceptions.PublishServeException(
          "System reserved word %s is used in target path %s." % (sub_path,
                                                                  t_path))

  def _CreatePublishRequest(self, origin_request_host, target_details):
    """Creates publish request.

    Args:
      origin_request_host: Origin request host (GEE server URL).
      target_details: dictionary of target information:
                      dbname, fusion_host, vhname, snippets, search defs, etc.
    Returns:
      request: The publish request.
    """
    request = http_io.Request()

    request.SetParameter("DbName", target_details["dbname"])
    request.SetParameter("TargetPath", target_details["targetpath"])
    request.SetParameter("VirtualHostName", target_details["vhname"])
    request.SetParameter("Host", target_details["fusion_host"])
    request.SetParameter("ServeWms", target_details["servewms"])

    # Check if publish context exists. For targets published using fusion server
    # 5.1.2 and earlier, there is no publish context.

    publish_context = target_details.get("publishcontext")
    if publish_context:
      value = publish_context.get("searchdefs")
      if value:
        if isinstance(value, list):
          value = ",".join(value)
        request.SetParameter("SearchDefName", value)

      value = publish_context.get("supsearchdefs")
      if value:
        if isinstance(value, list):
          value = ",".join(value)
        request.SetParameter("SupSearchDefName", value)

      value = publish_context.get("snippetsetname")
      if value:
        request.SetParameter("SnippetSetName", value)

      value = publish_context.get("poifederated", False)
      if value:
        request.SetParameter("PoiFederated", value)

    request.SetParameter(constants.ORIGIN_REQUEST_HOST, origin_request_host)
    request.SetParameter(constants.CMD, "PublishDb")

    return request

  def _CreateUnpublishRequest(self, origin_request_host, target_path):
    """Creates unpublish request.

    Args:
      origin_request_host: Origin request host (GEE server URL).
      target_path: Target path.
    Returns:
      request: The unpublish request.
    """
    request = http_io.Request()

    request.SetParameter(constants.ORIGIN_REQUEST_HOST, origin_request_host)
    request.SetParameter(constants.CMD, constants.CMD_UNPUBLISH_DB)
    request.SetParameter(constants.TARGET_PATH, target_path)

    return request

  def HandleRepublishRequest(self, request, response):
    """Handles republish database request.

    Sample request:
    '/geserve/Publish?Cmd=RepublishDb&Host=fusion.host.name&
    TargetPath=test&DbName=/gevol/assets/Databases/terrain_alpha_pack_test.kdata
    base/gedb.kda/ver00X/gedb'

    Args:
      request: request object.
      response: response object.
    Raises:
      PublishServeException.
    """

    logger.debug("HandleRepublishRequest...")

    # Extract parameters
    db_name = request.GetParameter(constants.DB_NAME)
    target_path_in = request.GetParameter(constants.TARGET_PATH)
    origin_request_host = request.GetParameter(constants.ORIGIN_REQUEST_HOST)
    client_host_name = request.GetParameter(constants.HOST_NAME)

    if not db_name or not target_path_in or not client_host_name:
      raise exceptions.PublishServeException(
          "HandleRepublishRequest:db_name, host_name or target_path "
          "parameters not available in the request: %s, %s, %s.", db_name,
          client_host_name, target_path_in)

    target_path = serve_utils.NormalizeTargetPath(target_path_in)
    if not target_path:
      raise exceptions.PublishServeException(
          "HandleRepublishRequest: Not a valid target path %s "
          "(path format is /sub_path1[/sub_path2]." % target_path_in)

    if self._publish_helper.IsDatabasePushed(client_host_name, db_name):
      target_details = self._publish_helper.GetTargetDetails(target_path)
      if not target_details:
        raise exceptions.PublishServeException(
            "HandleRepublishRequest: Make sure the target path %s "
            "exists and is currently published." % target_path)

      if "publishcontext" not in target_details.keys():
        raise exceptions.PublishServeException(
            "Republish is not supported for targets "
            "published using GEE version 5.1.2 or earlier.")

      # Check if the databases are comparable, versions of the same database.
      if not self._publish_helper.AreDatabasesComparable(
          db_name, client_host_name, target_details["dbname"],
          target_details["fusion_host"]):
        raise exceptions.PublishServeException(
            "HandleRepublishRequest: Database names do not match for target "
            "and given database. Target database: %s, %s, Input database: %s, "
            "%s should only be versions of the same database." %
            (target_details["fusion_host"],target_details["dbname"],
             client_host_name, db_name))

      # Check if db_name has POI data.
      # Get database ID from gesearch database.
      value = target_details.get("fusion_host", "")
      search_db_id = self._publish_helper.GetSearchDbId(value, db_name)

      if search_db_id == 0:
        # db_name has no POI data. Do not republish if DB published
        # on target has POISearch enabled.
        if "POISearch" in target_details["publishcontext"].get("searchdefs"):
          raise exceptions.PublishServeException(
              "HandleRepublishRequest: target_path %s has POISearch service "
              "enabled while the new version of database %s has no POISearch "
              "data. Republish is disabled." % (target_path, db_name))

      # Replace db name with the new one.
      target_details["dbname"] = db_name

      # Unpublish target.
      request_u = self._CreateUnpublishRequest(origin_request_host, target_path)
      response_u = http_io.Response()
      self.HandleUnpublishRequest(request_u, response_u)

      # Publish target with new db_name.
      request_p = self._CreatePublishRequest(origin_request_host,
                                             target_details)
      response_p = http_io.Response()
      self.HandlePublishRequest(request_p, response_p)

      logger.debug("Database %s has been successfully republished to "
                   "target %s.", db_name, target_path)

      http_io.ResponseWriter.AddBodyElement(response, constants.HDR_STATUS_CODE,
                                            constants.STATUS_SUCCESS)

  def HandleSwapTargetsRequest(self, request, response):
    """Handles swap targets request.

    Sample request:
    '/geserve/Publish?Cmd=SwapTargets&Host=fusion.host.name&
      TargetPathA=target_path_a&TargetPathB=target_path_b

    Args:
      request: request object.
      response: response object.
    Raises:
      PublishServeException.
    """
    logger.debug("HandleSwapTargetsRequest...")

    # Extract parameters.
    target_path_in_a = request.GetParameter(constants.TARGET_PATH_A)
    target_path_in_b = request.GetParameter(constants.TARGET_PATH_B)
    origin_request_host = request.GetParameter(constants.ORIGIN_REQUEST_HOST)

    target_path_a = serve_utils.NormalizeTargetPath(target_path_in_a)
    if not target_path_a:
      raise exceptions.PublishServeException(
          "HandleSwapTargetsRequest: Not a valid target path %s "
          "(path format is /sub_path1[/sub_path2]." % target_path_in_a)

    target_path_b = serve_utils.NormalizeTargetPath(target_path_in_b)
    if not target_path_b:
      raise exceptions.PublishServeException(
          "HandleSwapTargetsRequest: Not a valid target path %s "
          "(path format is /sub_path1[/sub_path2]." % target_path_in_b)

    (target_details_a, target_details_b) = self._publish_helper.SwapTargets(
        target_path_a, target_path_b)

    # Unpublish targets.

    # There is no need to handle any errors/exceptions here, as they
    # are handled by the 'UnPublish' service.
    request_u = self._CreateUnpublishRequest(origin_request_host, target_path_a)
    response_u = http_io.Response()
    self.HandleUnpublishRequest(request_u, response_u)

    request_u = self._CreateUnpublishRequest(origin_request_host, target_path_b)
    response_u = http_io.Response()
    self.HandleUnpublishRequest(request_u, response_u)

    # Publish targets with new db_name.
    request_a = self._CreatePublishRequest(origin_request_host,
                                           target_details_a)
    response_a = http_io.Response()
    self.HandlePublishRequest(request_a, response_a)

    request_b = self._CreatePublishRequest(origin_request_host,
                                           target_details_b)
    response_b = http_io.Response()
    self.HandlePublishRequest(request_b, response_b)

    logger.debug("Targets %s and %s have been successfully swapped.",
                 target_path_a, target_path_b)
    http_io.ResponseWriter.AddBodyElement(response, constants.HDR_STATUS_CODE,
                                          constants.STATUS_SUCCESS)


def main():
  pass


if __name__ == "__main__":
  main()
