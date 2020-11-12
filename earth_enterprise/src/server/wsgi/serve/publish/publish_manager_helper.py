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


"""Publish manager helper module.

Classes for handling publish requests on the low level (database, filesystem).
"""

import copy
import datetime
import json
import logging
import operator
import os
import re
import shutil
import subprocess
import tempfile
import urlparse
from inspect import getmembers
from pprint import pprint

from common import exceptions
from common import utils

import psycopg2

from serve import basic_types
from serve import constants
from serve import http_io
from serve import serve_utils
from serve import stream_manager
from serve.push.search.core import search_manager


logger = logging.getLogger("ge_stream_publisher")

HTACCESS_REWRITE_BASE = "\nRewriteBase /\n"

# Minimum portable globe size in MB.
GLOBE_SIZE_THRESHOLD = 1.0

# Redirect Earth Client requests with no Globe specified on the URL
# to the default_ge based URIs to provide Earth Client access to a default map 
# if none is specified in the server connection URLs. 
EC_DEFAULT_MAP_LINE0_LOCAL_REWRITECOND =  "RewriteCond %{HTTP_USER_AGENT}  ^EarthClient/(.*)$\n"
EC_DEFAULT_MAP_LINE1_GOOGLE_REDIRECT = (
    "Redirect '/dbRoot.v5' '/%s/dbRoot.v5'\n")
EC_DEFAULT_MAP_LINE2_GOOGLE_REDIRECT = (
    "Redirect '/flatfile'  '/%s/flatfile'\n" )

LINE0_TARGETDESCR = "\n# target: %s\n"
# Rewrite rule template for adding trailing slash.
LINE1_TRAILING_SLASH_REWRITERULE = "RewriteRule '^%s$'  '%s/'  [NC,R]\n"
# Rewrite rule template for POISearch serving.
LINE2_POISEARCH_REWRITERULE = "RewriteRule '^%s/%s(.*)'  %s$1 [NC,PT]\n"

# Rewrite rule templates for WMS serving.
WMS_LINE0_REWRITERULE_R404 = "RewriteRule '^%s/wms' - [NC,R=404]\n"
WMS_LINE0_REWRITECOND = "RewriteCond %{QUERY_STRING}  ^(.*)$\n"
WMS_LINE1_REWRITERULE = (
    "RewriteRule '^%s/wms'  'wms?%%1&TargetPath=%s' [NC,PT]\n")

# Rewrite rules templates for GE database requests serving.
GE_LINE0_REWRITERULE = "RewriteRule '^%s/+$'  earth/earth_local.html [NC,PT]\n"
GE_LINE1_REWRITECOND = "RewriteCond %{QUERY_STRING}  ^(.*)$\n"
GE_LINE2_REWRITERULE = (
    "RewriteRule '^%s/(.*)'  '%s%s/db/$1?%%1&db_type=%s' [NC]\n")

# Rewrite rules templates for Map database requests serving.
MAP_LINE0_LOCAL_REWRITERULE = (
    "RewriteRule '^%s/+$'  maps/maps_local.html [NC,PT]\n")
MAP_LINE0_GOOGLE_REWRITERULE = (
    "RewriteRule '^%s/+$'  maps/maps_google.html [NC,PT]\n")
MAP_LINE1_REWRITERULE = (
    "RewriteRule '^%s/+maps/+mapfiles/(.*)$'  maps/mapfiles/$1 [NC,PT]\n")
MAP_LINE2_REWRITECOND = "RewriteCond %{QUERY_STRING}  ^(.*)$\n"
MAP_LINE3_REWRITERULE = (
    "RewriteRule '^%s/(.*)'  '%s%s/db/$1?%%1&db_type=%s' [NC]\n")


# Rewrite rules templates for portable globes requests serving.
# GLB or 3d GLC
GLX_LINE0_REWRITERULE = (
    "RewriteRule '^%s/+$'  portable/preview.html?%s [NC,PT]\n")
GLX_LINE1_REWRITECOND = "RewriteCond %{QUERY_STRING}  ^(.*)$\n"
GLX_LINE2_REWRITERULE = (
    "RewriteRule '^%s/(.*)'  '%s%s/db/$1?%%1&db_type=%s' [NC]\n")


class PublishManagerHelper(stream_manager.StreamManager):
  """Class for handling publish requests."""

  VS_CONFIG_PATH = "/opt/google/gehttpd/conf.d/virtual_servers"

  HTACCESS_PATH = "/opt/google/gehttpd/htdocs/.htaccess"
  HTACCESS_TMP_PREFIX = "gepublish_htacces_"

  HTACCESS_GE_PUBLISH_BEGIN = "### GE_PUBLISH BEGIN\n"
  HTACCESS_GE_PUBLISH_END = "### GE_PUBLISH END\n"

  PUBLISH_PATH_TEMPL = "{0}{1}"
  TARGET_PATH_TEMPL = "{0}/targets{1}"

  def __init__(self):
    """Inits publish manager helper."""
    super(PublishManagerHelper, self).__init__()
    self._search_manager = search_manager.SearchManager()

  def BuildDbPublishPath(self, fusion_hostname, db_name):
    """Builds publish path for Fusion database.

    Args:
      fusion_hostname: Fusion hostname.
      db_name: database name (assetroot path).
    Returns:
      The complete publish path of specified Fusion database.
    """
    # Fusion hostname should be always defined for Fusion database.
    assert fusion_hostname
    return os.path.normpath(
        PublishManagerHelper.PUBLISH_PATH_TEMPL.format(
            self.GetFusionDbPublishPathPrefix(fusion_hostname), db_name))

  def BuildTargetPublishPath(self, db_publish_path, target_path):
    """Builds complete publish path for target point of Fusion database.

    Args:
      db_publish_path: publish path of database.
      target_path: target path.
    Returns:
      The complete publish path of specified target.
    """
    return os.path.normpath(
        PublishManagerHelper.TARGET_PATH_TEMPL.format(
            db_publish_path, target_path))

  def IsDefaultDatabase(self, publish_context_id):
    """
    Checks whether the passed-in database is the default database or not. When
    upgrading from older releases such as 5.1.2, the publish_context_table may
    not have an entry for a published database, so we have to perform two
    queries: one to get the list of databases and one to check if each database
    is the default. It would simplify things somewhat to move the ec_default_db
    field to the target_table database so that we can get all the data we want
    with a single query. However, this would make the upgrade code more
    complicated because we would have to manage 3 schemas: one from before we
    added ec_default_db and two with the ec_default_db in different places.
    This method seems to be the simplest overall option even though it requires
    multiple queries.
    """
    if publish_context_id != 0: # Ensure the publish_context_id is valid
      query_string = ("""
          SELECT publish_context_table.ec_default_db
          FROM publish_context_table
          WHERE publish_context_table.publish_context_id = %s AND
          publish_context_table.ec_default_db = TRUE
          """)
      results = self.DbQuery(query_string, (publish_context_id,))
      if results:
        return True
    return False

  def HandleQueryRequest(self, request, response):
    """Handles query requests.

    Args:
      request: request object.
      response: response object.
    Raises:
      psycopg2.Error/Warning, PublishServeException.
    """
    query_cmd = request.GetParameter(constants.QUERY_CMD)
    if not query_cmd:
      raise exceptions.PublishServeException("Missing Query Command.")

    # List all DBs registered on server.
    if query_cmd == constants.QUERY_CMD_LIST_DBS:
      self._GetDbsList(response)
    # TODO: Convert _GetAllAssets to _GetDbsList once
    #                the front end is ready to receive the new response.
    elif query_cmd == constants.QUERY_CMD_LIST_ASSETS:
      self._GetAllAssets(response)
    # List all Virtual Hosts registered on server.
    elif query_cmd == constants.QUERY_CMD_LIST_VSS:
      results = self.QueryVhList()
      for vh_name, vh_url, vh_ssl in results:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_VS_NAME, vh_name)
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_VS_URL,
            self._GetVhCompleteUrl(vh_url, vh_ssl))

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # Get Virtual Host details.
    elif query_cmd == constants.QUERY_CMD_VS_DETAILS:
      vh_name = request.GetParameter(constants.VS_NAME)
      if not vh_name:
        raise exceptions.PublishServeException("Missing virtual host name.")

      vh_url, vh_ssl = self.QueryVh(vh_name)
      vh_complete_url = self._GetVhCompleteUrl(vh_url, vh_ssl)
      if vh_complete_url:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_VS_URL, vh_complete_url)

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # List all target paths serving published databases.
    elif query_cmd == constants.QUERY_CMD_LIST_TGS:
      query_string = (
          "SELECT target_path FROM target_table WHERE target_id IN ("
          "SELECT target_id FROM target_db_table)")
      results = self.DbQuery(query_string)

      for line in results:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_TARGET_PATH, line)

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # Get target details.
    # TODO: consider to remove unnecessary details from response.
    # It might be QUERY_CMD_DB_DETAILS which return only DB info that we have
    # in postgres for specified target.
    # Note: in ListDbs we return all this information.
    elif query_cmd == constants.QUERY_CMD_PUBLISHED_DB_DETAILS:
      target_path = request.GetParameter(constants.TARGET_PATH)
      norm_target_path = serve_utils.NormalizeTargetPath(target_path)
      if not norm_target_path:
        raise exceptions.PublishServeException("Missing target path.")

      query_string = ("""
          SELECT db_table.host_name, db_table.db_name, db_table.db_pretty_name,
            db_table.db_timestamp AT TIME ZONE 'UTC', db_table.db_size, 
            virtual_host_table.virtual_host_name,
            virtual_host_table.virtual_host_url,
            virtual_host_table.virtual_host_ssl,
            target_table.target_path, target_table.serve_wms,
            target_db_table.publish_context_id
          FROM target_table, target_db_table, db_table, virtual_host_table
          WHERE target_table.target_path = %s AND
            target_table.target_id = target_db_table.target_id AND
            target_db_table.db_id = db_table.db_id AND
            target_db_table.virtual_host_id = virtual_host_table.virtual_host_id
          """)

      results = self.DbQuery(query_string, (norm_target_path,))

      if results:
        assert isinstance(results, list) and len(results) == 1
        (r_host_name, r_db_path, r_db_name, r_db_timestamp, r_db_size,
         r_virtual_host_name, r_virtual_host_url, r_virtual_host_ssl,
         r_target_path, r_serve_wms, publish_context_id) = results[0]
        r_default_database = self.IsDefaultDatabase(publish_context_id)

        db_info = basic_types.DbInfo()
        # TODO: make re-factoring - implement some Set function
        # to use it where it is needed. Maybe build an aux. dictionary and
        # pass as a parameter to that function.
        db_info.host = r_host_name      # db_table.host_name
        db_info.path = r_db_path        # db_table.db_name
        db_info.name = r_db_name        # db_table.db_pretty_name
        timestamp = r_db_timestamp      # db_table.db_timestamp
        if timestamp:
          assert isinstance(timestamp, datetime.datetime)
          db_info.timestamp = serve_utils.DatetimeNoTzToIsoFormatUtc(timestamp)
        db_info.size = r_db_size         # db_table.db_size
        db_info.description = r_db_name  # db_table.db_pretty_name
        db_info.virtual_host_name = r_virtual_host_name
        db_info.target_base_url = self.GetVhBaseUrl(r_virtual_host_url,
                                                    r_virtual_host_ssl)
        db_info.target_path = r_target_path
        db_info.serve_wms = r_serve_wms
        db_info.default_database = r_default_database
        db_info.registered = True

        # Calculate database attributes.
        serve_utils.CalcDatabaseAttributes(db_info)

        # Check whether the Fusion database has been pushed from remote host
        # and set corresponding flag in DbInfo.
        if serve_utils.IsFusionDb(db_info.type):
          db_info.remote = self._IsFusionDbRemote(db_info)

        # Set whether database has POI search data.
        search_db_id = self._search_manager.QueryDbId(
            db_info.host, db_info.path)
        db_info.has_poi = search_db_id != 0

        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DATA,
            json.dumps(db_info, cls=basic_types.DbInfoJsonEncoder))
      else:
        raise exceptions.PublishServeException(
            "Target path %s does not exist." % target_path)
    # Gets published DB path by target path.
    elif query_cmd == constants.QUERY_CMD_GEDB_PATH:
      query_target_path = request.GetParameter(constants.TARGET_PATH)
      norm_target_path = serve_utils.NormalizeTargetPath(query_target_path)
      if not norm_target_path:
        raise exceptions.PublishServeException("Missing target path.")

      query_string = ("""
          SELECT db_table.host_name, db_table.db_name, target_table.target_path
          FROM target_table, target_db_table, db_table
          WHERE target_table.target_path = %s AND
            target_table.target_id = target_db_table.target_id AND
            target_db_table.db_id = db_table.db_id
          """)

      results = self.DbQuery(query_string, (norm_target_path,))

      if results:
        assert isinstance(results, list) and len(results) == 1
        (client_host_name, db_path, target_path) = results[0]
        gedb_path = self.BuildDbPublishPath(client_host_name, db_path)
        target_gedb_path = self.BuildTargetPublishPath(
            gedb_path, target_path)
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DATA, target_gedb_path)
      else:
        raise exceptions.PublishServeException(
            "Target path '%s' does not exist." % query_target_path)
    elif query_cmd == constants.QUERY_CMD_TARGET_DETAILS:
      target_path_in = request.GetParameter(constants.TARGET_PATH)
      if not target_path_in:

        raise exceptions.PublishServeException(
            "Missing target path in the request.")

      target_path = serve_utils.NormalizeTargetPath(target_path_in)

      if not target_path:
        raise exceptions.PublishServeException(
            "Not a valid target path %s "
            "(path format is /sub_path1[/sub_path2]." % target_path)

      self.HandleTargetDetailsRequest(target_path, response)
    else:
      raise exceptions.PublishServeException(
          "Invalid Query Command: %s." % query_cmd)

  def HandlePublishRequest(self, db_id, publish_def):
    """Handles publish database request.

    Args:
      db_id: database ID.
      publish_def: The PublishDef object encapsulating
       set of the publish parameters.
    Raises:
      psycopg2.Error/Warning, PublishServeException.
    """
    target_path = publish_def.target_path
    virtual_host_name = publish_def.virtual_host_name
    db_type = publish_def.db_type
    client_host_name = publish_def.client_host_name
    serve_wms = publish_def.serve_wms
    snippets_set_name = publish_def.snippets_set_name
    search_defs = publish_def.search_tabs
    sup_search_defs = publish_def.sup_search_tabs
    ec_default_db = publish_def.ec_default_db
    poifederated = publish_def.poi_federated

    assert target_path and target_path[0] == "/" and target_path[-1] != "/"

    # Check if the VS template exists.
    virtual_host_id = self._QueryVirtualHostId(virtual_host_name)
    if virtual_host_id == -1:
      raise exceptions.PublishServeException(
          "Virtual host %s does not exist." % virtual_host_name)

    transfer_file_paths = self.SynchronizeDb(db_id, db_type, client_host_name)
    if not transfer_file_paths:
      # Add target point into target_table.
      target_id = self._AddTarget(target_path, serve_wms)

      # clear the default DB flag if this publish has one set, so that 
      # the older database is not considered the default.  
      if ec_default_db:
        logger.warn( "Database %s will be set as the default when Earth Client connections when no database is specified." % target_path )
        query_string = ("UPDATE publish_context_table"
                        " SET ec_default_db = FALSE ")
        result = self.DbModify(query_string)

      # Insert publish context into 'publish_context_table' table.
      query_string = ("INSERT INTO publish_context_table"
                      " (snippets_set_name, search_def_names,"
                      " supplemental_search_def_names, poifederated, ec_default_db)"
                      " VALUES(%s, %s, %s, %s, %s) RETURNING"
                      " publish_context_id")
 
      result = self.DbModify(
          query_string,
          (snippets_set_name, search_defs, sup_search_defs, poifederated, ec_default_db),
          returning=True)

      publish_context_id = 0
      if result:
        publish_context_id = result[0]

      # Note: target is not removed from target_table in case of
      # any exception below.
      # Link target point with VS template, database and publish context.
      query_string = ("INSERT INTO target_db_table"
                      " (target_id, virtual_host_id, db_id, publish_context_id)"
                      " VALUES(%s, %s, %s, %s)")
      self.DbModify(query_string,
                    (target_id, virtual_host_id, db_id, publish_context_id))
    else:
      raise exceptions.PublishServeException("Database is not pushed.")

  def HandleUnpublishRequest(self, target_path):
    """Handles un-publish database request.

    Deletes the entry in target_db_table, target_search_id_table,
    publish_context_table, updates .htaccess file,
    deletes target's publish directory.
    Note: target is not removed from target_table

    Args:
      target_path: target path to un-publish.
    Raises:
      psycopg2.Error/Warning, PublishServeException.
    """
    unused_host_name, db_name = self.DoUnpublish(target_path)
    if not db_name:
      raise exceptions.PublishServeException(
          "There is no database associated with target path %s." % (
              target_path))

  def DoUnpublish(self, target_path):
    """Do unpublish specified target path.

    Args:
      target_path: target path to un-publish.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      (fusion_host_name, db_name): unpublished database info.
    """
    # Check if target exists.
    # Note: Here we have case-sensitive query for target from target_table.
    # It allows to keep target path as user have entered it. Client gets target
    # path to unpublish from Server.
    target_id = self._QueryTargetIdByPath(target_path)
    if target_id == -1:
      logger.warning(
          "HandleUnpublishRequest: The target path %s does not exist.",
          target_path)
      return None, None

    publish_context_id = self._QueryPublishContextId(target_id)

    # Only try delete from the publish_context_table for
    # a valid non zero publish_context_id.
    if publish_context_id != 0:
      # Delete the entry in 'publish_context_table' table.
      query_string = ("DELETE FROM publish_context_table "
                      "WHERE publish_context_id = %s")
      num_rows = self.DbModify(query_string, (publish_context_id,))

    # Get db_name before deleting a corresponding entry in the
    # target_db_table.
    (unused_virtual_host_url, db_name, fusion_host_name,
     unused_db_flags) = self._QueryTargetDetailsById(target_id)

    # Delete the entry in target_db_table.
    query_string = "DELETE FROM target_db_table WHERE target_id = %s"
    num_rows = self.DbModify(query_string, (target_id,))

    if num_rows:
      # Remove un-published target from .htaccess by updating .htaccess file.
      self.UpdateHtaccessFile()

    if db_name:
      # Delete target's publish directory.
      self.DeleteTargetPublishDir(target_path, fusion_host_name, db_name)

    return fusion_host_name, db_name

  def IsTargetPathUsed(self, target_path):
    """Checks whether specific target path is in use.

    Note: The check is case-insensitive, since we make target path(URL-path)
    case insensitive. We do not allow to have two of the same published points,
    while keeping a target path in database as user have entered it.

    Args:
      target_path: target path.
    Returns:
      whether target path is in use.
    Raises:
      psycopg2.Error/Warning.
    """
    query_string = ("""
        SELECT 1 FROM target_table, target_db_table
        WHERE lower(target_table.target_path) = %s AND
          target_table.target_id = target_db_table.target_id
        LIMIT 1""")

    result = self.DbQuery(query_string, (target_path.lower(),))
    if result:
      return True

    return False

  def DeleteTargetPublishDir(self, target_path, client_host_name, db_name):
    """Deletes target's publish directory.

    Args:
      target_path: target path.
      client_host_name: client host name.
      db_name: database name (assetroot path).
    Raises:
      PublishServeException.
    """
    (norm_db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)
    if serve_utils.IsFusionDb(db_type):
      if not client_host_name:
        raise exceptions.PublishServeException(
            "Internal error - undefined host name for Fusion database %s." %
            db_name)

      gedb_path = self.BuildDbPublishPath(client_host_name, norm_db_path)
      target_gedb_path = self.BuildTargetPublishPath(gedb_path, target_path)

      try:
        logger.debug("Delete DB publish directory: %s", target_gedb_path)
        # Remove all the files/dirs under the publish db path (included).
        shutil.rmtree(target_gedb_path)
      except OSError as e:
        logger.warning(
            "HandleUnpublishRequest: Could not delete DB publish directory: %s"
            ", Error: %s", target_gedb_path, e)

      try:
        # Remove '..gedb/targets'- directory if it is empty.
        os.rmdir(os.path.dirname(target_gedb_path))
      except OSError:
        pass

  def Cleanup(self):
    """Cleans up publisher (publish info tables).

    Un-publishes Fusion DBs, portable globes that do not exist on filesystem.
    Returns:
      list of unpublished Fusion DBs/portables [{host:, path:},..]
    """
    # Get information about published DBs/globes.
    query_string = (
        """SELECT db_table.host_name, db_table.db_name, db_table.db_pretty_name,
               target_table.target_path
           FROM target_db_table, db_table, target_table
           WHERE target_table.target_id = target_db_table.target_id AND
               db_table.db_id = target_db_table.db_id
        """)

    results = self.DbQuery(query_string)

    unpublished_dbs = []

    # Flag for whether globes directory is mounted and at least one portable
    # globe exists. If not, don't remove Portables from postgres db.
    is_globes_mounted = (
        os.path.exists(constants.CUTTER_GLOBES_PATH) and
        serve_utils.ExistsPortableInDir(
            constants.CUTTER_GLOBES_PATH))

    if not is_globes_mounted:
      logger.warning(
          "HandleCleanupRequest: No portable files in directory %s."
          " Volume may not be mounted.",
          constants.CUTTER_GLOBES_PATH)
      logger.warning("Portable globe publish records have not been cleaned.")

    for line in results:
      # Get database type.
      (db_path, db_type) = serve_utils.IdentifyPublishedDb(line[1])

      do_clean_up = False
      if serve_utils.IsFusionDb(db_type):
        db_host = line[0]
        publish_db_path = self.BuildDbPublishPath(db_host, db_path)
        publish_db_path = "{0}/header.xml".format(publish_db_path)
        db_name = serve_utils.GetFusionDbInfoName(line[2], db_type)
        do_clean_up = True
      else:
        assert serve_utils.IsPortable(db_type)
        if is_globes_mounted:
          publish_db_path = "{0}{1}".format(
              constants.CUTTER_GLOBES_PATH, db_path)
          db_name = line[1]
          db_host = ""
          do_clean_up = True
        else:
          logger.warning("%s does not exist. Volume may not be mounted.",
                         PublishManagerHelper.CUTTER_GLOBES_PATH)

      target_path = line[3]
      if do_clean_up and not os.path.exists(publish_db_path):
        self.DoUnpublish(target_path)
        unpublished_dbs.append({"host": db_host, "path": db_path})
        logger.warning(
            "The database/portable globe '{0}' could not be found."
            " The path '{1}' serving it has been un-published.".format(
                db_name, target_path))

    logger.info("Publish info cleanup is complete.")
    return unpublished_dbs

  def _QueryTargetDbDetailsByPath(self, target_path):
    """Queries target details by target path.

    Args:
      target_path: target path.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      target details as tuple (virtual_host_url, db_name, host_name).
      (None, None, None) tuple is returned in case of no DB published to
      this target.
    """
    assert target_path and target_path[0] == "/" and target_path[-1] != "/"

    target_details = {}
    query_string = ("""SELECT db_table.host_name, db_table.db_name,
              virtual_host_table.virtual_host_name, target_table.serve_wms
              FROM target_table, target_db_table, db_table, virtual_host_table
              WHERE target_table.target_path = %s AND
              target_table.target_id = target_db_table.target_id AND
              target_db_table.db_id = db_table.db_id AND
              target_db_table.virtual_host_id =
              virtual_host_table.virtual_host_id""")

    result = self.DbQuery(query_string, (target_path,))

    if result:
      assert isinstance(result[0], tuple)
      (db_host_name, db_name, virtual_host_name, servewms) = result[0]
      target_details.update({
          "servewms": servewms,
          "fusion_host": db_host_name,
          "dbname": db_name,
          "vhname": virtual_host_name,
      })

    return target_details

  def _QueryPublishContextByTargetPath(self, target_path):
    """Queries gestream database to get publish_context for target path.

    Args:
      target_path : target path.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      publish_context as dict with fields {snippetssetname:string,
      searchdefs:[string,], supsearchdefs:[string,], poifederated:bool}.

    """
    publish_context = {}
    query_string = ("""SELECT publish_context_table.snippets_set_name,
           publish_context_table.search_def_names,
           publish_context_table.supplemental_search_def_names,
           publish_context_table.poifederated
           FROM target_table, target_db_table, publish_context_table
           WHERE target_table.target_path = %s AND
                 target_table.target_id = target_db_table.target_id AND
                 target_db_table.publish_context_id =
                 publish_context_table.publish_context_id""")

    result = self.DbQuery(query_string, (target_path,))

    if result:
      assert isinstance(result[0], tuple)
      (snippets_set_name, search_def_names, sup_search_def_names,
       poifederated) = result[0]
      publish_context.update({
          "snippetsetname": snippets_set_name,
          "searchdefs": search_def_names,
          "supsearchdefs": sup_search_def_names,
      })

      if "POISearch" in search_def_names:
        publish_context["poifederated"] = poifederated

    return publish_context

  def _QueryPublishContextId(self, target_id):
    """Queries publish_context_id from target_db_table.

    Args:
      target_id: target path Id.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      Publish context id.
    """
    publish_context_id = 0
    query_string = ("SELECT publish_context_id FROM target_db_table "
                    "WHERE target_id = %s")
    result = self.DbQuery(query_string, (target_id,))
    if result:
      publish_context_id = int(result[0])

    return publish_context_id

  def _QueryTargetDetailsById(self, target_id):
    """Queries target details by target ID.

    Args:
      target_id: target ID.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      target details as tuple (virtual_host_url, db_name, host_name).
      (None, None, None) tuple is returned in case of no DB published to
      this target.
    """
    virtual_host_url = None
    db_name = None
    host_name = None
    db_flags = None

    query_string = ("""
        SELECT virtual_host_table.virtual_host_url, db_table.db_name,
          db_table.host_name, db_table.db_flags
        FROM target_db_table, virtual_host_table, db_table
        WHERE target_db_table.target_id = %s AND
          virtual_host_table.virtual_host_id =
              target_db_table.virtual_host_id AND
          db_table.db_id = target_db_table.db_id""")

    result = self.DbQuery(query_string, (target_id,))
    if result:
      assert isinstance(result[0], tuple)
      (virtual_host_url, db_name, host_name, db_flags) = result[0]

    return (virtual_host_url, db_name, host_name, db_flags)

  def HandleAddVsRequest(self,
                         vs_name, vs_url, vs_ssl, vs_cache_level,
                         response):
    """Handles add virtual server request.

    Args:
      vs_name: the virtual server name.
      vs_url: the virtual server URL.
      vs_ssl: whether it is SSL virtual server.
      vs_cache_level: the virtual server cache level.
      response: the response object.
    Raises:
      psycopg2.Error/Warning, PublishServeException
    """
    # Check if virtual host already exists.
    if self._QueryVirtualHostId(vs_name) != -1:
      raise exceptions.PublishServeException(
          "HandleAddVsRequest: Virtual host %s already exists." % vs_name)

    # We do not check if the corresponding config file exists. This because
    # we don't know how our users might want to name that file.

    # Add the virtual host entry.
    query_string = (
        "INSERT INTO virtual_host_table (virtual_host_name,"
        " virtual_host_url, virtual_host_ssl, virtual_host_cache_level)"
        " VALUES(%s, %s, %s, %s)")
    self.DbModify(query_string, (vs_name, vs_url, vs_ssl, vs_cache_level))
    # Create virtual server config file.
    vs_url_complete = self._GetVhCompleteUrl(vs_url, vs_ssl)
    self._CreateVsConfig(vs_name, vs_url_complete)
    self._RestartServers()
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def HandleDeleteVsRequest(self, vs_name, response):
    """Handles delete virtual server request.

    Args:
      vs_name: virtual host name.
      response: response object.
    Raises:
      psycopg2.Error/Warning, PublishServeException
    """
    # Check if virtual server exists and is disabled. There is no database
    # published on this virtual server.
    if self._QueryVsUsed(vs_name):
      raise exceptions.PublishServeException(
          "HandleDeleteVsRequest: Make sure the virtual host %s"
          " exists and is currently not being used." % vs_name)

    # Delete the entry in virtual_host_table.
    query_string = "DELETE FROM virtual_host_table WHERE virtual_host_name = %s"
    self.DbModify(query_string, [vs_name])
    self._RestartServers()
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def GetPublishInfoList(self):
    """Gets publish info list.

    Returns:
      list of tuples (target_path, host_name, db_name).
    """
    query_string = """
        SELECT target_path, host_name, db_name
          FROM target_table, db_table, target_db_table
          WHERE
            target_table.target_id = target_db_table.target_id AND
            db_table.db_id = target_db_table.db_id"""
    return self.DbQuery(query_string)

  def GetSearchDefDetails(self, search_def_name):
    return self._search_manager.GetSearchDefDetails(search_def_name)

  def GetSearchDbId(self, client_host_name, db_name):
    return self._search_manager.QueryDbId(client_host_name, db_name)

  def GetVsUrlPathList(self):
    query_string = (
        "SELECT virtual_host_url FROM virtual_host_table")
    results = self.DbQuery(query_string)
    vh_list = []
    for vh_url in results:
      url_parse_res = urlparse.urlparse(vh_url)
      vh_list.append(url_parse_res.path)

    return vh_list

  def GetCutSpecs(self):
    """Gets cut specifications.

    Returns:
      list of cut specifications.
    """
    results = self.DbQuery("""
        SELECT name, qtnodes, exclusion_qtnodes,
               min_level, default_level, max_level
          FROM cut_spec_table""")
    return results

  def UpdateHtaccessFile(self):
    """Updates .htaccess file."""
    # Get a list of (target_path, target_id) pairs from target_table.
    target_paths_list = self._ListTargetPaths()
    if not target_paths_list:
      return

    # Sort by target path in descending order.
    # Note: The order in which these rules are defined is
    # important - this is the order in which they will be applied at run-time.
    # Sorting  in descending order is necessary to prevent usurping by shorter
    # paths that would match first.
    get_key = operator.itemgetter(0)
    target_paths_list.sort(key=lambda elem: get_key(elem).lower(), reverse=True)

    # Write publish content into .htaccess.
    out_file = tempfile.NamedTemporaryFile(
        mode="w+",
        prefix=PublishManagerHelper.HTACCESS_TMP_PREFIX,
        delete=False)
    try:
      if os.path.exists(PublishManagerHelper.HTACCESS_PATH):
        is_publish_content_added = False
        with open(PublishManagerHelper.HTACCESS_PATH, mode="r") as in_file:
          in_publish_section = False
          for line in in_file:
            if line == PublishManagerHelper.HTACCESS_GE_PUBLISH_BEGIN:
              in_publish_section = True
              self._WritePublishContentToHtaccessFile(
                  out_file, target_paths_list)
              is_publish_content_added = True
            elif line == PublishManagerHelper.HTACCESS_GE_PUBLISH_END:
              in_publish_section = False
              continue

            if not in_publish_section:
              out_file.write(line)
        if not is_publish_content_added:
          self._WritePublishContentToHtaccessFile(out_file, target_paths_list)
      else:
        self._WritePublishContentToHtaccessFile(out_file, target_paths_list)
    except Exception:
      out_file.close()
      os.unlink(out_file.name)
      raise
    else:
      # Copy temp htaccess file into htdocs rewriting existing .htaccess.
      out_file.close()
      shutil.copyfile(out_file.name, PublishManagerHelper.HTACCESS_PATH)
      os.unlink(out_file.name)

  def _AddTarget(self, target_path, serve_wms):
    """Adds target path into target_table and sets serve_wms flag.

    Args:
      target_path: target path.
      serve_wms: whether target point is servable through WMS.
    Raises:
      psycopg2.Error/Warning, PublishServeException.
      PublishServeException is raised in case of this target path is already
      in use.
    Returns:
      target_id: ID of added/existed target point.
    """
    assert target_path and target_path[0] == "/" and target_path[-1] != "/"
    # Check if the target point already exists.
    # Note: Here we have case-sensitive query for target from target_table.
    # It allows to keep target path as user have entered it.
    target_id = self._QueryTargetIdByPath(target_path)
    if target_id != -1:
      # Check if the target point is currently used.
      if self._QueryIsTargetUsed(target_id):
        # Note: might be an assert since we check it before.
        raise exceptions.PublishServeException(
            "Target path %s is already in use. Note that paths are "
            "case insensitve. Input another path"
            " or un-publish database using this path." % target_path)

      # Sets serve_wms flag for existing path.
      query_string = ("UPDATE target_table SET serve_wms = %s"
                      " WHERE target_path = %s")
      self.DbModify(query_string, (serve_wms, target_path))
      return target_id

    # Add the target point entry.
    query_string = (
        "INSERT INTO target_table (target_path, serve_wms) VALUES(%s, %s)")
    self.DbModify(query_string, (target_path, serve_wms))
    target_id = self._QueryTargetIdByPath(target_path)
    return target_id

  def _QueryVirtualHostId(self, virtual_host_name):
    """Queries Virtual Host ID by name.

    Args:
      virtual_host_name: name of Virtual Host.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      ID of Virtual Host in case of it exists, otherwise -1.
    """
    query_string = ("SELECT virtual_host_id FROM virtual_host_table"
                    " WHERE virtual_host_name = %s")
    result = self.DbQuery(query_string, (virtual_host_name,))
    virtual_host_id = -1
    if result:
      virtual_host_id = int(result[0])

    return virtual_host_id

  def _QueryVirtualHostIdAndDbId(self, target_id):
    """Queries Virtual Host ID and Db ID by target ID.

    Args:
      target_id: target ID.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      tuple (virtual_host_id, db_id). If there is no DB published on
      specified target then it returns tuple (None, None).
    """
    query_string = ("SELECT virtual_host_id, db_id FROM target_db_table"
                    " WHERE target_id = %s")
    result = self.DbQuery(query_string, (target_id,))
    virtual_host_id = None
    db_id = None
    if result:
      assert isinstance(result[0], tuple)
      virtual_host_id = int(result[0][0])
      db_id = int(result[0][1])

    return (virtual_host_id, db_id)

  def _QueryTargetIdByPath(self, target_path):
    """Queries target point ID by its path.

    Note: query is case-sensitive since we keep target path in database as user
    have entered it.

    Args:
      target_path: target point path.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      ID of target point in case of it exists otherwise -1.
    """
    query_string = "SELECT target_id FROM target_table WHERE target_path = %s"
    result = self.DbQuery(query_string, (target_path,))
    target_id = -1
    if result:
      target_id = int(result[0])

    return target_id

  def _QueryIsTargetUsed(self, target_id):
    """Queries whether target point is taken.

    Args:
      target_id: target point ID.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      whether target point with specified target_id is used.
    """
    is_target_used = False
    query_string = "SELECT db_id FROM target_db_table WHERE target_id = %s"
    result = self.DbQuery(query_string, (target_id,))
    if result:
      is_target_used = True

    return is_target_used

  def _QueryDbAndHostName(self, db_id):
    """Queries database name and host name by database ID.

    Args:
      db_id: database ID.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      (db_name, host_name) tuple. Values are None in case database with
      specified db_id does not exist.
    """
    host_name = None
    db_name = None
    if db_id == 0:
      return (db_name, host_name)
    query_string = "SELECT db_name, host_name FROM db_table WHERE db_id = %s"

    result = self.DbQuery(query_string, (db_id,))
    if result:
      assert isinstance(result[0], tuple)
      db_name = result[0][0]
      host_name = result[0][1]

    return (db_name, host_name)

  def _QueryVsUsed(self, vs_name):
    """Queries whether virtual server is used.

    Virtual server is used - there is a database served with it.

    Args:
      vs_name: virtual server name.
    Returns:
      whether virtual server is used.
    Raises:
      psycopg2.Error/Warning
    """
    query_string = (
        "SELECT db_id FROM target_db_table WHERE virtual_host_id IN ("
        "SELECT virtual_host_id FROM virtual_host_table"
        " WHERE virtual_host_name = %s)")
    results = self.DbQuery(query_string, (vs_name,))

    if results:
      return True
    return False

  def _GetDbsList(self, response):
    """Gets list of available databases.

    Args:
      response: response object.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      list of DbInfo objects serialized into json response object.
    """
    # TODO: try-except here is a temporary solution.
    # Move to DoGet() when all responses are formatted in json.
    try:
      database_list, unused_set = self._GetDatabaseList()
      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, database_list)
    except exceptions.PublishServeException as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except psycopg2.Warning as w:
      logger.error(w)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(w))
    except psycopg2.Error as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except Exception as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(
          response, "Server-side Internal Error: {0}".format(e))

  def _GetAllAssets(self, response):
    """Gets list of available fusion databases and portables.

    Args:
      response: response object.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      list of databases and portables serialized into json response object.
    """
    try:
      results, registered_portable_set = self._GetDatabaseList()
      results.extend(self._GetPortableGlobesList(registered_portable_set))
      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, results)
    except exceptions.PublishServeException as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except psycopg2.Warning as w:
      logger.error(w)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(w))
    except psycopg2.Error as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except Exception as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(
          response, "Server-side Internal Error: {0}".format(e))

  # TODO: add database description in Fusion and handle it here.
  def _GetDatabaseList(self):
    """Gets list of Fusion databases.

    Raises:
      psycopg2.Error/Warning.
    Returns:
      tuple (list of DbInfo objects, list of registered portable names).
    """
    # Get database information from db_table.
    query_string = ("SELECT db_id, host_name, db_name, db_pretty_name,"
                    " db_timestamp AT TIME ZONE 'UTC', db_size FROM db_table")

    results = self.DbQuery(query_string)
    # Holder for registered portable names.
    registered_portable_set = set()

    # Parsing results into DbInfo list.
    db_info_list = []
    db_id_list = []
    for line in results:
      db_id_list.append(int(line[0]))  # db_table.db_id
      db_info = basic_types.DbInfo()
      db_info.host = line[1]           # db_table.host_name
      db_info.path = line[2]           # db_table.db_name
      db_info.name = line[3]           # db_table.db_pretty_name
      timestamp = line[4]                       # db_table.db_timestamp
      if timestamp:
        assert isinstance(timestamp, datetime.datetime)
        db_info.timestamp = serve_utils.DatetimeNoTzToIsoFormatUtc(timestamp)
      db_info.size = line[5]                    # db_table.db_size
      db_info.description = line[3]             # db_table.db_pretty_name
      db_info.registered = True
      # Determine database features.
      serve_utils.CalcDatabaseAttributes(db_info)

      # Check whether the Fusion database has been pushed from remote host and
      # set corresponding flag in DbInfo.
      if serve_utils.IsFusionDb(db_info.type):
        db_info.remote = self._IsFusionDbRemote(db_info)

      # Store name of registered portables to avoid list duplicates.
      if serve_utils.IsPortable(db_info.type):
        storage_name = (db_info.name[1:] if db_info.name[0] == "/" else
                        db_info.name)
        registered_portable_set.add(storage_name)
      db_info_list.append(db_info)

    # Set whether database has POI search data.
    search_dbs_set = set(self._search_manager.QueryListDbs())
    if search_dbs_set:
      for db_info in db_info_list:
        db_info.has_poi = (db_info.host, db_info.path) in search_dbs_set

    # Get auxiliary dictionary mapping a database ID to a publish info list.
    db_to_publish_info = self._GetDbIdToPublishInfoDict()

    # Get auxiliary dictionary mapping a Virtual Host name to a base URL.
    vhname_to_baseurl = self._GetVhNameToBaseUrlDict()

    # For published databases in DbInfo list, set publish info:
    # virtual host name, target base URL, target path, serve_wms.
    # Note: we get additional db_info-s in case we have
    # databases that are published to more then one target.
    add_db_info_list = []
    for db_id, db_info in zip(db_id_list, db_info_list):
      if db_id in db_to_publish_info:
        publish_info_list = db_to_publish_info[db_id]
        publish_info = publish_info_list[0]
        db_info.virtual_host_name = publish_info[0]
        db_info.target_base_url = vhname_to_baseurl[db_info.virtual_host_name]
        db_info.target_path = publish_info[1]
        db_info.serve_wms = publish_info[2]
        db_info.default_database = publish_info[3]
        if len(publish_info_list) > 1:
          for vh_name, target_path, serve_wms, default_database in publish_info_list[1:]:
            add_db_info = copy.copy(db_info)
            add_db_info.virtual_host_name = vh_name
            add_db_info.target_base_url = vhname_to_baseurl[
                add_db_info.virtual_host_name]
            add_db_info.target_path = target_path
            add_db_info.serve_wms = serve_wms
            add_db_info.default_database = default_database
            add_db_info_list.append(add_db_info)
    db_info_list.extend(add_db_info_list)

    return (db_info_list, registered_portable_set)

  def _GetPortableGlobesList(self, registered_portable_set):
    """Gets portable globes list.

    Scans cutter/globes directory and sub-directories for all the glx files
    located there.  First checks for registered portables and ignores them;
    They are already added in _GetDatabaseList().

    Args:
      registered_portable_set: set of registered portable names.
    Returns:
      list of (unregistered) Portable globes.
    """

    # Build a list of portable globes.
    globes_list = []
    root = constants.CUTTER_GLOBES_PATH
    for name in os.listdir(root):
      # Ignore globes that are registered.
      if name not in registered_portable_set:
        if os.path.isfile(os.path.join(root, name)):
          db_info = basic_types.DbInfo()
          db_info.name = name
          db_info.type = db_info.name[-3:]
          # Ignore files that are not Portables, eg .README
          if serve_utils.IsPortable(db_info.type):
            serve_utils.GlxDetails(db_info)
            if db_info.size > GLOBE_SIZE_THRESHOLD:
              globes_list.append(db_info)
        else:
          logger.warn( "%s is not a valid file and is being ignored." % os.path.join(root,name) )

    return globes_list

  def _CreateVsConfig(self, vs_name, vs_url):
    """Writes virtual server config for specified virtual host.

    Args:
      vs_name: virtual server name.
      vs_url: virtual server URL.
    """
    logger.debug("_CreateVsConfig...")
    url_parse_res = urlparse.urlparse(vs_url)

    if url_parse_res.scheme == "https":
      vs_config_file_path = os.path.normpath(
          os.path.join(
              PublishManagerHelper.VS_CONFIG_PATH,
              (vs_name + "_host.location_ssl")))
      self._WriteSslVsConfig(
          vs_config_file_path, vs_name, url_parse_res.path)
    else:
      vs_config_file_path = os.path.normpath(
          os.path.join(
              PublishManagerHelper.VS_CONFIG_PATH,
              (vs_name + "_host.location")))
      self._WriteVsConfig(
          vs_config_file_path, vs_name, url_parse_res.path)

  def _WriteVsConfig(self, vs_config_file_path, vs_name, vs_path):
    """Write default content to VS config file.

    Args:
      vs_config_file_path: config file path.
      vs_name: virtual server name.
      vs_path: virtual server path (location).
    """
    with open(vs_config_file_path, "w") as f:
      f.write("# The virtual host %s.\n" % vs_name)
      f.write("RewriteEngine on\n\n")
      f.write("<Location %s/>\n" % vs_path)
      f.write("  SetHandler fdb-handler\n")
      f.write("</Location>\n")

  def _WriteSslVsConfig(self, vs_config_file_path, vs_name, vs_path):
    """Write default content to SSL VS config.

    Args:
      vs_config_file_path: config file path.
      vs_name: virtual server name.
      vs_path: virtual server path (location).
    """
    with open(vs_config_file_path, "w") as f:
      f.write("# The SSL virtual host %s.\n" % vs_name)
      f.write("RewriteEngine on\n\n")
      f.write("<Location %s/>\n" % vs_path)
      f.write("  SetHandler fdb-handler\n")
      f.write("  SSLRequireSSL\n")
      f.write("  SSLVerifyClient none\n")
      f.write("</Location>\n")

  def _RestartServers(self):
    """Restart servers.

    Raises:
      PublishServeException
    """
    logger.debug("_RestartServers...")
    try:
      # Reload Apache configs
      cmd_reload = "/opt/google/bin/gerestartapache"
      logger.info("Earth Server restarting...")
      subprocess.Popen([cmd_reload, ""])
    except Exception as e:
      raise exceptions.PublishServeException(e)

  def _ListTargetPaths(self):
    """Gets target paths serving published databases.

    Raises:
      psycopg2.Error/Warning.
    Returns:
      list of tuples (target_path, target_id, serve_wms).
    """
    query_string = (
        "SELECT target_path, target_id, serve_wms FROM target_table"
        " WHERE target_id IN (SELECT target_id FROM target_db_table)")
    return self.DbQuery(query_string)

  # Finds the database that is the default stream for Earth Clients.
  def _GetEcDefaultDbTargetPath(self):
    """Gets target paths serving published databases.

    Raises:
      psycopg2.Error/Warning.
    Returns:
      list of tuples (target_path, target_id, serve_wms).
    """
    target_path_result = None
    query_string = (
        """SELECT target_table.target_path
           FROM publish_context_table, target_table, target_db_table
           WHERE publish_context_table.ec_default_db = TRUE AND
                 target_table.target_id = target_db_table.target_id AND
                 target_db_table.publish_context_id =
                 publish_context_table.publish_context_id;""")
    results = self.DbQuery(query_string)
    if results:
      if isinstance(results, list) and len(results) > 0:
        ( target_path_result ) = results[0]
    return target_path_result

  def _WritePublishContentToHtaccessFile(self, htaccess_file,
                                         target_paths_list):
    """Writes publish content into htaccess-file.

    Args:
      htaccess_file: file descriptor for writing to.
      target_paths_list: target paths list.
    Raises:
      psycopg2.Error/Warning, PublishServeException.
    """
    default_target_path = self._GetEcDefaultDbTargetPath() 

    # Write publish header to file.
    htaccess_file.write("%s" % PublishManagerHelper.HTACCESS_GE_PUBLISH_BEGIN)
    # Write RewriteBase to file.
    htaccess_file.write("%s" % HTACCESS_REWRITE_BASE)

    logger.info( "Default target path is currently set to: %s " % default_target_path)
    if default_target_path: 
      # Database is set to default for Earth Client:
      relative_target_path = default_target_path[1:]
      htaccess_file.write( EC_DEFAULT_MAP_LINE0_LOCAL_REWRITECOND )
      htaccess_file.write( EC_DEFAULT_MAP_LINE1_GOOGLE_REDIRECT % relative_target_path )
      htaccess_file.write( EC_DEFAULT_MAP_LINE2_GOOGLE_REDIRECT % relative_target_path )

    # Collects all the needed information for all the target paths based on
    # target ID and adds corresponding rewrite rules into htacces-file.
    for (target_path, target_id, serve_wms) in target_paths_list:
      (virtual_host_url,
       db_name, host_name, db_flags) = self._QueryTargetDetailsById(target_id)
      if (not virtual_host_url) or (not db_name):
        continue   # no DB published on this target path.

      # Identify type of published DB.
      (unused_norm_db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)

      if serve_utils.IsFusionDb(db_type):
        if not host_name:
          raise exceptions.PublishServeException(
              "Internal Error - undefined host name for Fusion database %s." %
              db_name)
      else:
        assert serve_utils.IsPortable(db_type)
        if host_name:
          raise exceptions.PublishServeException(
              "Internal Error - host name is not empty for portable %s." %
              db_name)

      # Put the rules into htacces file for current target
      url_parse_res = urlparse.urlparse(virtual_host_url)
      virtual_host_path = url_parse_res.path
      relative_target_path = target_path[1:]

      # Common lines for all the databases, globes.
      htaccess_file.write(LINE0_TARGETDESCR % target_path)
      htaccess_file.write(LINE1_TRAILING_SLASH_REWRITERULE % (
          relative_target_path, relative_target_path))
      htaccess_file.write(LINE2_POISEARCH_REWRITERULE % (
          relative_target_path,
          constants.POI_SEARCH_SERVICE_NAME,
          constants.POI_SEARCH_SERVICE_NAME))
      if serve_wms:
        htaccess_file.write(WMS_LINE0_REWRITECOND)
        htaccess_file.write(WMS_LINE1_REWRITERULE % (
            relative_target_path, target_path))
      else:
        htaccess_file.write(WMS_LINE0_REWRITERULE_R404 % (
            relative_target_path))

      # Content for Fusion earth (GE database).
      if db_type == basic_types.DbType.TYPE_GE:
        htaccess_file.write(GE_LINE0_REWRITERULE % relative_target_path)
        htaccess_file.write(GE_LINE1_REWRITECOND)
        htaccess_file.write(GE_LINE2_REWRITERULE % (
            relative_target_path, virtual_host_path, target_path, db_type))
    
      # Content for Fusion map (map database).
      elif db_type == basic_types.DbType.TYPE_MAP:
        assert isinstance(db_flags, int)
        if db_flags & basic_types.DbFlags.USE_GOOGLE_BASEMAP == 0:
          htaccess_file.write(MAP_LINE0_LOCAL_REWRITERULE %
                              relative_target_path)
        else:
          htaccess_file.write(MAP_LINE0_GOOGLE_REWRITERULE %
                              relative_target_path)

        htaccess_file.write(MAP_LINE1_REWRITERULE % relative_target_path)
        htaccess_file.write(MAP_LINE2_REWRITECOND)
        htaccess_file.write(MAP_LINE3_REWRITERULE % (
            relative_target_path, virtual_host_path, target_path, db_type))
      # Content for portable globes.
      elif serve_utils.IsPortable(db_type):
        htaccess_file.write(GLX_LINE0_REWRITERULE % (
            relative_target_path, target_path))
        htaccess_file.write(GLX_LINE1_REWRITECOND)
        htaccess_file.write(GLX_LINE2_REWRITERULE % (
            relative_target_path, virtual_host_path, target_path, db_type))
      else:
        raise exceptions.PublishServeException(
            "Unsupported DB type %s.", db_type)

    # write publish footer to file.
    htaccess_file.write("\n%s" %PublishManagerHelper.HTACCESS_GE_PUBLISH_END)

  def _GetDbIdToPublishInfoDict(self):
    """Builds a dictionary mapping a database ID to a publish info list.

    Returns:
      dictionary mapping a database ID to a publish info list -
      {db_id: [(vh_name, target_path, serve_wms),]}
    """
    # Get (db_id, target paths, virtual_host) tuples for all published dbs.
    query_db_target = (
        "SELECT target_db_table.db_id,"
        " virtual_host_table.virtual_host_name,"
        " target_table.target_path, target_table.serve_wms,"
        " target_db_table.publish_context_id"
        " FROM target_db_table, target_table, virtual_host_table"
        " WHERE target_table.target_id = target_db_table.target_id AND"
        " virtual_host_table.virtual_host_id = target_db_table.virtual_host_id")
    results = self.DbQuery(query_db_target)

    # Build a dictionary.
    db_to_publish_info_dct = dict(
        (db_id, []) for (db_id, unused_vh_name, unused_target_path,
                         unused_serve_wms, unused_default_db) in results)
    for db_id, vh_name, target_path, serve_wms, publish_context_id in results:
      default_database = self.IsDefaultDatabase(publish_context_id)
      db_to_publish_info_dct[db_id].append(
          (vh_name, target_path, serve_wms, default_database))

    return db_to_publish_info_dct

  def _GetVhNameToBaseUrlDict(self):
    """Builds a dictionary mapping a virtual host name to a base URL.

    Returns:
      dictionary mapping a virtual host name to a base URL
      (scheme://host[:port]) -  {vh_name: vh_base_url}
    """
    vh_list = self.QueryVhList()
    vhname_to_baseurl_dct = {}
    for (vh_name, vh_url, vh_ssl) in vh_list:
      vhname_to_baseurl_dct[vh_name] = self.GetVhBaseUrl(vh_url, vh_ssl)

    return vhname_to_baseurl_dct

  def GetVhBaseUrl(self, vh_url, vh_ssl):
    """Builds a Virtual Host base URL.

    If the vh_url is scheme://host:port/path, then it extracts
    scheme://host:port to build a base URL, otherwise (vh_url is a path,
    e.g. /public) it builds a base URL based on information in Apache config
    and FQDN.

    Args:
      vh_url: virtual host URL - /path or scheme://host:[port]/path.
      vh_ssl: whether virtual host is SSL.
    Raises:
      PublishServeException.
    Returns:
      virtual host base URL - scheme://host[:port]
    """
    url_parse_res = urlparse.urlparse(vh_url)
    if url_parse_res.scheme and url_parse_res.netloc:
      return "{0}://{1}".format(url_parse_res.scheme, url_parse_res.netloc)
    else:
      # VH URL is specified as absolute path, then build VH base URL based on
      # information in Apache config.
      scheme_host_port = utils.GetApacheSchemeHostPort()
      if not scheme_host_port:
        raise exceptions.PublishServeException(
            "Unable to build Server URL based on Apache config.")
      else:
        assert len(scheme_host_port) == 3
        (scheme, host, port) = scheme_host_port
        assert scheme
        assert host
        assert port
        # override scheme in according with VH properties.
        scheme = "https" if vh_ssl else "http"
        host = "localhost" if vh_url == "/local_host" else host
        vh_base_url = "{0}://{1}".format(scheme, host)

        # Note: Do not pick up port from Apache config for SSL virtual host,
        # use default port if SSL virtual host specified with absolute path.

        if (not vh_ssl) and port and port != "80":
          # Get port number for not SSL virtual host from Apache config and
          # put it into URL if it is not default.
          vh_base_url += ":{0}".format(port)
        return vh_base_url

  def _GetVhCompleteUrl(self, vh_url, vh_ssl):
    """Builds a Virtual Host complete URL.

    If the vh_url is scheme://host:port/path, then just return vh_url,
    otherwise (vh_url is an absolute path, e.g. /public) it builds a VH URL
    based on information in Apache config and FQDN.

    Args:
      vh_url: virtual host URL - /path or scheme://host:[port]/path.
      vh_ssl: whether virtual host is SSL.
    Raises:
      PublishServeException.
    Returns:
      virtual host complete URL - scheme://host[:port]/path
    """
    vh_base_url = self.GetVhBaseUrl(vh_url, vh_ssl)
    return urlparse.urljoin(vh_base_url, vh_url)

  def _IsFusionDbRemote(self, db_info):
    """Determines whether a Fusion database has been pushed from remote host.

    Args:
      db_info: database info.
    Returns:
      whether a Fusion database has been pushed from remote Fusion host.
    """
    assert serve_utils.IsFusionDb(db_info.type)
    return db_info.host != self.server_hostname

  def GetTargetDetails(self, target_path):
    """gets target details by target path.

    Args:
      target_path: target path.
    Raises:
      PublishServeException
    Returns:
      target details of a target.
    """
    target_details = {}
    target_db_details = {}
    publish_context = {}

    target_db_details = self._QueryTargetDbDetailsByPath(target_path)
    if not target_db_details:
      error_msg = (
          "GetTargetDetails: No target details found for target path %s. "
          "Make sure target path exists." % target_path)
      raise exceptions.PublishServeException(error_msg)

    target_details.update({
        "targetpath": target_path,
        "servewms": target_db_details["servewms"],
        "fusion_host": target_db_details["fusion_host"],
        "dbname": target_db_details["dbname"],
        "vhname": target_db_details["vhname"],
    })

    publish_context = self._QueryPublishContextByTargetPath(target_path)

    if publish_context:
      target_details.update({"publishcontext": publish_context,})

    return target_details

  def HandleTargetDetailsRequest(self, target_path, response):
    """Handles 'targetdetails' request.

    Args:
      target_path: target path.
      response: response object.
    Raises:
      PublishServeException
    """
    target_details = self.GetTargetDetails(target_path)

    if not target_details:
      raise exceptions.PublishServeException(
          "HandleTargetDetailsRequest: The publish target %s does not exist." %
          target_path)

    http_io.ResponseWriter.AddBodyElement(response, constants.HDR_STATUS_CODE,
                                          constants.STATUS_SUCCESS)

    for key, value in target_details.iteritems():
      if key == "publishcontext":
        for publish_context_key, publish_context_value in value.iteritems():
          http_io.ResponseWriter.AddBodyElement(
              response, constants.HDR_DATA,
              "%s : %s" % (publish_context_key, publish_context_value))
      else:
        http_io.ResponseWriter.AddBodyElement(response, constants.HDR_DATA,
                                              "%s : %s" % (key, value))

  def IsDatabasePushed(self, client_host_name, db_name):
    """Check if the database is pushed.

    Args:
      client_host_name: Request originating host name.
      db_name: Fusion database name.
    Raises:
      PublishServeException
    Returns:
       "True" if the database is pushed.
    """

    (unused_db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)

    # Check if the DB exists.
    db_id = self.QueryDbId(client_host_name, db_name)
    if db_id == 0:
      raise exceptions.PublishServeException(
          "Database %s does not exist on server.\n"
          "It needs to be registered/pushed before publishing." % db_name)

    # Check if the DB is pushed.
    if self.SynchronizeDb(db_id, db_type, client_host_name):
      error_msg = ("Database %s does not exist on server. It needs to be "
                   "registered/pushed before publishing." % db_name)
      logger.error(error_msg)
      raise exceptions.PublishServeException(error_msg)

    return True

  def SwapTargets(self, target_path_a, target_path_b):
    """Check if the targets can be swapped.

    Args:
      target_path_a: First target path.
      target_path_b: Second target path.
    Raises:
      PublishServeException
    Returns:
       Publish Context of both targets.
    """
    # Check if the target paths are the same.
    if target_path_a == target_path_b:
      raise exceptions.PublishServeException(
          "HandleSwapTargetsRequest:target paths %s and %s are same." %
          (target_path_a, target_path_b))

    # Get target details for target_path_a.
    target_details_a = self.GetTargetDetails(target_path_a)

    if not target_details_a:
      raise exceptions.PublishServeException(
          "HandleSwapTargetsRequest: Make sure the target path %s "
          "exists and is currently published." % target_path_a)

    if "publishcontext" not in target_details_a.keys():
      error_msg = ("SwapTargets: publish context does not exist "
                   "for target path %s. This command is not supported "
                   "for databases published with GEE 5.1.2 or earlier." %
                   target_path_a)
      raise exceptions.PublishServeException(error_msg)

    # Get target details for target_path_b.
    target_details_b = self.GetTargetDetails(target_path_b)

    if not target_details_b:
      raise exceptions.PublishServeException(
          "HandleSwapTargetsRequest: Make sure the target path '%s' "
          "exists and is currently published." % target_path_b)

    if "publishcontext" not in target_details_b.keys():
      error_msg = (
          "SwapTargets: publish context does not exist "
          "for target path %s. This command is not supported for databases "
          "pubished using older version of fusion." % target_path_b)
      raise exceptions.PublishServeException(error_msg)

    # Swap target paths.
    t_path = target_details_a["targetpath"]
    target_details_a["targetpath"] = target_details_b["targetpath"]
    target_details_b["targetpath"] = t_path

    return (target_details_a, target_details_b)

  def AreDatabasesComparable(self, db_name1, host_name1, db_name2, host_name2):
    """Check if the databases are same.

    Args:
      db_name1: First database.
      host_name1 : GEE host where db_name1 is published.
      db_name2: Second database.
      host_name2 : GEE host where db_name2 is published.
    Returns:
       boolean value depending on whether databases are comparable or not.
    """
    if host_name1 != host_name2:
      return False

    p = re.compile(r".*/(.*)/.*\.kda/.*")

    match = p.search(db_name1)
    if match:
      dbname_1 = match.groups()[0]
      match = p.search(db_name2)
      if match:
        dbname_2 = match.groups()[0]
        return dbname_1 == dbname_2

    return False

def main():
  pass


if __name__ == "__main__":
  main()
