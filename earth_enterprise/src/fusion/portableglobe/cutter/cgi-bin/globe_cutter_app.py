#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
# Copyright 2020 Open GEE Contributors
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

"""Supports web interface for cutting a globe based on a specified polygon.

Ajax calls are made for the individual steps so that the user can
get feedback on progress of process.

TODO: May be necessary to add a status call for longer processes
TODO: on bigger globes. Can use size of directory as a metric.
"""

import cgi
import os
import re
import shutil
import sys
import time
import ssl
import urllib
import urllib2
from contextlib import closing

import defusedxml.ElementTree as etree
import xml.etree.ElementTree as etree2

from common import form_wrap
from common import postgres_manager_wrap
import common.utils
from core import globe_cutter
import common.configs

CONFIG_FILE = "/opt/google/gehttpd/cgi-bin/advanced_cutter.cfg"
CONFIGS = common.configs.Configs(CONFIG_FILE)

COMMAND_DIR = "/opt/google/bin"
WEB_URL_BASE = "/cutter/globes"
WEB_DIR = "/opt/google/gehttpd/htdocs%s" % WEB_URL_BASE
TEMPLATE_DIR = "/opt/google/gehttpd/htdocs/cutter/template"
MAP_FILE_TEMPLATE = "%s/%%s.glm" % WEB_DIR
GLOBE_FILE_TEMPLATE = "%s/%%s.glb" % WEB_DIR
GLOBE_NAME_TEMPLATE = "%s_%s"

# Default values if no environment variables are set.
DEFAULT_PORTABLE_PORT = "9335"
DEFAULT_PORTABLE_SERVER = "localhost"
DEFAULT_PORTABLE_PREFIX = "http"
DEFAULT_SOURCE_GLOBE = ""
# Should be relative address, so appended to target base
# address (publish point) from which dbRoot was fetched.
DEFAULT_SEARCH_SERVICE = "Portable3dPoiSearch"

# Names of environment variables.
PORTABLE_SERVER_PARAM = "FORCE_PORTABLE_SERVER"
PORTABLE_PORT_PARAM = "FORCE_PORTABLE_PORT"
PORTABLE_PREFIX_PARAM = "FORCE_PORTABLE_PREFIX"
SEARCH_SERVICE_PARAM = "FORCE_SEARCH_SERVICE"
KML_SERVER_PARAM = "FORCE_KML_SERVER"
KML_PORT_PARAM = "FORCE_KML_PORT"
SOURCE_GLOBE_PARAM = "FORCE_SOURCE_GLOBE"
PORTABLE_TMP_PARAM = "FORCE_PORTABLE_TMP"

FORM = form_wrap.FormWrap(cgi.FieldStorage(), do_sanitize=True)
TMP_DIR = FORM.getvalue_path(PORTABLE_TMP_PARAM)
if not TMP_DIR:
  TMP_DIR = WEB_DIR

BASE_DIR = "%s/.globe_builder" % TMP_DIR
GLOBE_ENV_DIR_TEMPLATE = "%s/%%s_%%s" % BASE_DIR
GLOBE_FINAL_ENV_DIR_TEMPLATE = "%s/%%s_env" % BASE_DIR
LOG_FILE = "%s/log" % GLOBE_ENV_DIR_TEMPLATE
GLOBE_DIR_TEMPLATE = "%s/%%s" % GLOBE_ENV_DIR_TEMPLATE
ICONS_DIR_TEMPLATE = "%s/%%s/icons" % GLOBE_ENV_DIR_TEMPLATE
PLUGIN_DIR_TEMPLATE = "%s/%%s/earth" % GLOBE_ENV_DIR_TEMPLATE
MAPS_DIR_TEMPLATE = "%s/%%s/maps" % GLOBE_ENV_DIR_TEMPLATE
JSON_EARTH_FILE_TEMPLATE = "%s/%%s/earth/earth.json" % GLOBE_ENV_DIR_TEMPLATE
JSON_MAP_FILE_TEMPLATE = "%s/%%s/maps/map.json" % GLOBE_ENV_DIR_TEMPLATE
INFO_FILE_TEMPLATE = "%s/%%s/earth/info.txt" % GLOBE_ENV_DIR_TEMPLATE
JS_DIR_TEMPLATE = "%s/%%s/js" % GLOBE_ENV_DIR_TEMPLATE
KML_MAP_FILE_TEMPLATE = "%s/kml_map.txt" % GLOBE_ENV_DIR_TEMPLATE
KML_DIR_TEMPLATE = "%s/%%s/kml" % GLOBE_ENV_DIR_TEMPLATE
ICONS_DIR_TEMPLATE = "%s/%%s/icons" % GLOBE_ENV_DIR_TEMPLATE
SEARCH_DIR_TEMPLATE = "%s/%%s/search_db" % GLOBE_ENV_DIR_TEMPLATE
DBROOT_FILE_TEMPLATE = "%s/dbroot.v5" % GLOBE_ENV_DIR_TEMPLATE
DBROOT_DIR_TEMPLATE = "%s/%%s/dbroot" % GLOBE_ENV_DIR_TEMPLATE
DBROOT_FILE2_TEMPLATE = "%s/%%s/dbroot/dbroot_%%s_%%s" % GLOBE_ENV_DIR_TEMPLATE
POLYGON_FILE_TEMPLATE = "%s/%%s/earth/polygon.kml" % GLOBE_ENV_DIR_TEMPLATE
PACKET_INFO_TEMPLATE = "%s/packet_info.txt" % GLOBE_ENV_DIR_TEMPLATE
QTNODES_FILE_TEMPLATE = "%s/qt_nodes.txt" % GLOBE_ENV_DIR_TEMPLATE
METADATA_FILE_TEMPLATE = "%s/%%s/earth/metadata.json" % GLOBE_ENV_DIR_TEMPLATE

# Disk space minimum in MB before we start sending warnings.
DISK_SPACE_WARNING_THRESHOLD = 1000.0

# Default KML namespace
DEFAULT_KML_NAMESPACE = 'http://www.opengis.net/kml/2.2'

class OsCommandError(Exception):
  """Thrown if os command fails."""
  pass


class DiskFullError(Exception):
  """Thrown if disk partition is too full."""
  pass


class GlobeBuilder(object):
  """Class that implements all commands for cutting a globe."""

  @staticmethod
  def Query(db, query, parameters=None):
    """Submits the query to the database and returns tuples.

    Args:
      db: The database being queried.
      query: SQL SELECT statement.
      parameters: sequence of parameters to populate placeholders in SQL
                  statement.
    Returns:
      Results as list of lists (rows of fields).
    """
    return postgres_manager_wrap.PostgresManagerWrap.Query(
        db, query, parameters)

  @staticmethod
  def TableColumns(db, table):
    """Returns list of column names for the given table in the given db."""
    query = ("SELECT column_name FROM INFORMATION_SCHEMA.columns "
             "WHERE table_name=%s")
    return GlobeBuilder.Query(db, query, (table,))

  @staticmethod
  def PrintTable(column_names, data):
    """Save table data to a file."""
    fix_geom = False

    # Write out the column names.
    # Change the_geom column to lat and lon columns.
    if column_names[-1] == "the_geom":
      column_names[-1] = "lon"
      column_names.append("lat")
      fix_geom = True
    print "\t".join(column_names)

    # Write out each row of data.
    # Convert POINT() data to a lat and lon column.
    for row in data:
      if fix_geom:
        out_row = list(row[:-1])
        point = row[-1]
        lonlat = point[6:-1].split(" ")
        out_row.extend(lonlat)
        print "\t".join(out_row)
      else:
        print "\t".join(row)

  @staticmethod
  def Status(message):
    """Outputs a status message."""
    print "<br>%s" % common.utils.HtmlEscape(message)

  @staticmethod
  def StatusWarning(message):
    """Outputs a status message."""
    print ("<br><span class='text_status_warn'>%s</span>" %
           common.utils.HtmlEscape(message))

  def AddGlobeDirectory(self, description):
    """Add directory where globe will be built."""
    # Add early for info file.
    try:
      os.makedirs(self.plugin_dir)
    except os.error:
      pass  # Directory may already exist

    try:
      os.makedirs(self.dbroot_dir)
    except os.error:
      pass  # Directory may already exist

    self.CreateInfoFile()
    if description:
      self.AppendInfoFile("Globe description: %s" % description)
      self.Status("Description: %s" % description)
    else:
      self.Status("No description given.")

    self.logger.Log("Added globe directory: %s" % self.globe_dir)
    self.Status("Added globe directory: %s" % self.globe_dir)

  def SavePolygon(self, polygon):
    """Save polygon kml to a file."""
    with open(self.polygon_file, "w") as fp:
      if polygon:
        # Check XML validity and standardize representation
        contents = etree.fromstring(polygon)
        # empty namespace will just contain the string "kml"
        # otherwise, it will be in the format "{<schema>}kml"
        ns = DEFAULT_KML_NAMESPACE
        if len(contents.tag) > 3:
          ns = contents.tag[1:len(contents.tag)-4]  
        etree2.register_namespace('', ns)
        xml = etree2.ElementTree(contents)
        xml.write(fp, xml_declaration=True, encoding='UTF-8')
        self.Status("Saved polygon to %s" % self.polygon_file)
      else:
        self.Status("Created empty polygon file %s" % self.polygon_file)

  def ConvertPolygonToQtNodes(self, polygon_level, is_mercator=False):
    """Convert polygon into a set of qt nodes at given level."""
    self.Status("Convert polygon to quadtree nodes ...")
    try:
      os.remove(self.qtnodes_file)
    except OSError:
      pass  # Ok, if file isn't there.

    os_cmd = ("%s/gepolygontoqtnodes --qt_nodes_file=\"%s\" "
              "--kml_polygon_file=\"%s\" --max_level=%d"
              % (COMMAND_DIR, self.qtnodes_file, self.polygon_file,
                 polygon_level))
    if is_mercator:
      os_cmd += " --mercator"

    common.utils.ExecuteCmd(os_cmd, self.logger)
    fp = open(self.qtnodes_file)
    self.Status("%d qtnodes" % len(fp.readlines()))
    fp.close()

  @staticmethod
  def ConvertPolygonToPsql(polygon):
    """Convert polygon kml into PostGIS polygon text representation.

    Args:
      polygon: polygon in kml format.
    Returns:
      PostGIS polygon text representation string.
    """
    if not polygon:
      return ""

    parsed_polygon = polygon[polygon.find("<Polygon>"):
                             polygon.find("</Polygon>")].strip()
    parsed_polygon = (parsed_polygon[parsed_polygon.find("<coordinates>")
                                     + len("<coordinates>"):
                                     parsed_polygon.find("</coordinates>")])
    parsed_polygon = parsed_polygon.strip()
    coordinates = parsed_polygon.split(" ")
    if len(coordinates) < 4:
      return ""

    # Note: item of coordinates-list may have 2 or 3 coordinates.
    postgis_polygon = ", ".join(
        ["%s %s" % tuple(coord.split(",")[:2]) for coord in coordinates])

    postgis_polygon = "POLYGON((%s))" % postgis_polygon
    return postgis_polygon

  def RewriteDbRoot(self, source, include_historical):
    """Executes command to rewrite the dbroot and extract the icons it uses."""
    self.Status("Rewrite dbroot ...")

    historical_flag = '--disable_historical'
    if include_historical:
      historical_flag = ''

    os_cmd = ("%s/gerewritedbroot --source=\"%s\" --icon_directory=\"%s\" "
              "--dbroot_file=\"%s\" --search_service=\"%s\" "
              "--kml_map_file=\"%s\" "
              "%s"
              % (COMMAND_DIR, source, self.icons_dir, self.dbroot_file,
                 self.search_service, self.kml_map_file,
                 historical_flag))

    common.utils.ExecuteCmd(os_cmd, self.logger)
    self.Status("%d icons" % len(os.listdir(self.icons_dir)))

    os_cmd = ("cp \"%s\" \"%s\""
              % (self.dbroot_file, self.dbroot_file2))
    common.utils.ExecuteCmd(os_cmd, self.logger)

  def GrabKml(self, source):
    """Recursively grabs all kml files referenced in the dbroot."""
    self.Status("Grab kml files ...")
    os_cmd = (("%s/gekmlgrabber --kml_map_file=\"%s\" --output_directory=\"%s\""
               " --source=\"%s\"")
              % (COMMAND_DIR, self.kml_map_file, self.kml_dir, source))
    common.utils.ExecuteCmd(os_cmd, self.logger)
    self.Status("%d kml files" % len(os.listdir(self.kml_dir)))

  def BuildGlobe(self, source, default_level, max_level):
    """Executes command to cut globe and save data into packet bundles."""
    self.Status("Build globe ...")
    # Run this task as a background task.
    os_cmd = ("%s/geportableglobebuilder --source=\"%s\" --default_level=%d "
              "--max_level=%d --hires_qt_nodes_file=\"%s\" "
              "--globe_directory=\"%s\" --dbroot_file=\"%s\" --metadata_file=\"%s\" >\"%s\""
              % (COMMAND_DIR, source, default_level, max_level,
                 self.qtnodes_file, self.globe_dir, self.dbroot_file,
                 self.metadata_file,
                 self.packet_info_file))
    common.utils.ExecuteCmdInBackground(os_cmd, self.logger)

  def BuildMap(self, source, default_level, max_level, ignore_imagery_depth):
    """Executes command to cut map and save data into packet bundles."""
    self.Status("Build map ...")
    ignore_imagery_depth_str = str()
    if ignore_imagery_depth:
        ignore_imagery_depth_str = "--ignore_imagery_depth"
    # Run this task as a background task.
    # Having trouble with permissions if output is redirected to a file.
    os_cmd = ("%s/geportablemapbuilder "
              "%s "
              "--source=\"%s\" "
              "--hires_qt_nodes_file=\"%s\" "
              "--map_directory=\"%s\"  --default_level=%d --max_level=%d "
              "--metadata_file=\"%s\" "
              % (COMMAND_DIR, ignore_imagery_depth_str, source,
                 self.qtnodes_file, self.globe_dir, default_level,
                 max_level, self.metadata_file))

    common.utils.ExecuteCmdInBackground(os_cmd, self.logger)

    # TODO: Get real packet numbers for imagery and vectors.
    fp = open(self.packet_info_file, "w")
    fp.write("1 0 0")
    fp.close()

  def CreateInfoFile(self):
    """Create globe info file."""
    fp = open(self.info_file, "w")
    fp.write("Portable Globe\n")
    fp.write("Copyright 2017 Google Inc.\nLicensed under the Apache License, Version 2.0.\n")
    fp.write("portable %s:%s\n" % (self.portable_server, self.portable_port))
    fp.write("search %s\n" % (self.search_service))
    fp.write("kml %s:%s\n" % (self.kml_server, self.kml_port))
    fp.write("%s\n" % common.utils.GmTimeStamp())
    fp.close()

  def AppendInfoFile(self, message):
    """Create globe info file."""
    self.logger.Log(message)
    fp = open(self.info_file, "a")
    fp.write(common.utils.TimeStamp())
    fp.write("%s\n\n" % message)
    fp.close()

  def HttpGet(self, url):
    """Make an HTTP or HTTPS request with a context for checking or ignoring
    certificate errors depending on Config settings.
    """
    context = None
    response_data = ""
    http_status_code = 0

    try:
      # TODO: When Python 2.7 is used on Centos6, this if version<=2.6 block can be removed
      # and the 'else' ssl.SSLContext based block can be used instead.
      if sys.version_info[0] == 2 and sys.version_info[1] <= 6:
        with closing(urllib2.urlopen(url)) as fp:
          http_status_code = fp.getcode()
          response_data = fp.read()
      else:
        # Set the context based on cert requirements
        if CONFIGS.GetBool("VALIDATE_CERTIFICATE"):
          cert_file = CONFIGS.GetStr("CERTIFICATE_CHAIN_PATH")
          key_file = CONFIGS.GetStr("CERTIFICATE_KEY_PATH")
          context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
          context.load_cert_chain(cert_file, keyfile=key_file)
        else:
          context = ssl.create_default_context()
          context.check_hostname = False
          context.verify_mode = ssl.CERT_NONE
        with closing(urllib2.urlopen(url, context=context)) as fp:
          http_status_code = fp.getcode()
          response_data = fp.read()

    except:
      GlobeBuilder.StatusWarning("FAILED: Caught exception reading {0}".format(url))
      raise

    return (response_data, http_status_code)

  def AddJsonFile(self, source, is_map, portable_server, json_file):
    """Get JSON from server and add it to the portable globe plugin files.

    Args:
      source: server URL.
      is_map: whether it is a 2d map server/database:
              true - 2d map, false - 3d globe.
      portable_server: portable server URL.
      json_file: path to json file to write in.
    Returns:
      updated {maps,earth}.json content.
    """
    # Get JSON from the server.
    url = "%s/query?request=Json&var=geeServerDefs" % source
    self.Status("Rewrite JSON from: %s to: %s" % (url, json_file))

    json, code = self.HttpGet(url)
    if (code != 200):
      raise Exception("GET {0} failed with {1}".format(url, code))

    # Replace all urls to point at the portable server.
    start = 0
    new_json = ""
    is_altered = False
    for match in re.finditer(r"([\w\-]+)\s*:\s*\"http[s]?:"
                             "//[\w\-\.]+(:\d+)?/([^/\n]+)([^\n\"]*)\"",
                             json, 0):
      spans = match.span()
      new_json += json[start:spans[0]]
      new_json += ("%s : \"%s%s\"" %
                   (match.groups()[0], portable_server, match.groups()[3]))
      start = spans[1]
      is_altered = True
    new_json += json[start:]
    json = new_json

    # Replace the serverUrl parameter with the plain Portable Server url.
    # This allows simple replacement by the Portable Server depending on
    # the context.
    new_json = ""
    start = 0
    for match in re.finditer(r"serverUrl\s*:\s*\"[^\n\"]*\"", json, 0):
      spans = match.span()
      new_json += json[start:spans[0]]
      new_json += ("serverUrl : \"%s\"" %  portable_server)
      start = spans[1]
      is_altered = True
      break
    new_json += json[start:]

    if is_map:
      if new_json.find("ImageryMapsMercator") >= 0:
        # MotF is not supported in Portable Server, and we don't need it
        # since we are getting the tiles already converted to the Mercator
        # projection.
        new_json = new_json.replace("ImageryMapsMercator", "ImageryMaps")
        is_altered = True

      # Get search tabs from server and insert into ServerDefs.
      search_tabs = globe_cutter.GlobeCutter.GetSearchTabs(source)
      if search_tabs:
        combined_json = globe_cutter.GlobeCutter.AddSearchTabsToServerDefs(
            new_json, search_tabs)
        if combined_json:
          new_json = combined_json
          is_altered = True

    if not is_altered:
      print "Json has not been altered."

    # Write modified JSON to portable server file.
    fp = open(json_file, "w")
    fp.write(new_json)
    fp.close()

    return new_json

  def AddJsonIcons(self, source, json):
    """Get icons from JSON and add them to the map."""
    # Add icon directory if needed.
    try:
      os.makedirs(self.icons_dir)
    except os.error:
      pass  # Directory may already exist

    # Get all of the icons from the json, ignoring duplicates.
    icons = {}
    for match in re.finditer("icon\s*:\s*\"icons/(.*)\"",
                             json, 0):
      icons[match.groups()[0]] = True

    for icon in icons.iterkeys():
      # Get JSON from the server.
      url = "%s/query?request=Icon&icon_path=icons/%s" % (source, icon)
      try:
        data, code = self.HttpGet(url)
        if code == 200:
          with open("%s/%s" % (self.icons_dir, icon), "w") as fpw:
            fpw.write(data)
        else:
          raise Exception("Cannot fetch {0}".format(url))
      except:
        self.Status("Unable to retrieve icon %s" % icon)

  def AddPluginFiles(self, source, is_map):
    """Copies files associated with the Google Earth browser plug-in."""
    self.Status("Add plugin files ...")
    # Plugin directory should already exist.
    common.utils.CopyDirectory("%s/earth" % TEMPLATE_DIR, self.plugin_dir,
                               self.logger)
    common.utils.CopyDirectory("%s/maps" % TEMPLATE_DIR, self.maps_dir,
                               self.logger)
    common.utils.CopyDirectory("%s/js" % TEMPLATE_DIR, self.js_dir, self.logger)

    # Get the Json that defines the plugin params.
    if is_map:
      json = self.AddJsonFile(
          source, is_map, self.json_address, self.json_map_file)
      self.AddJsonIcons(source, json)
    else:
      self.AddJsonFile(source, is_map, self.json_address, self.json_earth_file)

  def BuildSearchDb(self, source, polygon):
    """Extracts database info needed for POI search."""
    self.Status("Extract search data ...")
    try:
      os.makedirs(self.search_dir)
    except os.error:
      pass  # Directory may already exist

    # Determine the server and target path (fusion db publish point) from
    # the source.
    target = ""
    server = ""

    if source:
      server, target = common.utils.GetServerAndPathFromUrl(source)

    # Replace the server with advanced configuration host
    server = CONFIGS.GetStr("DATABASE_HOST")

    target = common.utils.NormalizeTargetPath(target)
    base_url = "%s/cgi-bin/globe_cutter_app.py" % server

    url = "%s?cmd=POI_IDS&target=%s" % (base_url, target)
    self.Status("Querying search poi ids: target=%s" % target)
    poi_list = None
    try:
      data, http_status_code = self.HttpGet(url)
      if http_status_code == 200:
        poi_list = data.strip()
    except Exception as e:
      raise Exception("Request failed: cannot connect to server: {0}".format(e))

    if poi_list:
      # Quote polygon parameter for URI.
      polygon_quoted = ""
      if polygon:
        polygon_quoted = urllib.quote(polygon)

      poi_ids = poi_list.split(" ")
      for poi_id in poi_ids:
        url = ("%s?cmd=SEARCH_FILE&poi_id=%s&polygon=%s" %
               (base_url, poi_id, polygon_quoted))
        search_file = "%s/gepoi_%s" % (self.search_dir, poi_id)
        try:
          self.Status("Querying search poi data: poi_id=%s, polygon=%s" %
                      (poi_id, polygon))
          data, http_status_code = self.HttpGet(url)
          if http_status_code == 200:
            self.Status("Copying search poi data: gepoi_%s to globe" % poi_id)

            with open(search_file, "w") as fpw:
              fpw.write(data.strip())
              fpw.write("\n")
          else:
            self.StatusWarning(fp.read())

          fp.close()

        except IOError as e:
          self.StatusWarning(
              "Unable to write search file: %s. Error: %s" % (search_file, e))
        except Exception as e:
          self.StatusWarning("Unable to get search data: gepoi_%s. Error: %s" %
                             (poi_id, e))
    else:
      self.Status("No search data.")

  def BuildSearchFile(self, poi_id, polygon):
    """Extracts database info needed for POI search.

    Args:
      poi_id: index of POI table.
      polygon: area of interest (polygon in kml-format).
    """
    # Converts polygon from kml format to PostGIS.
    postgis_polygon = GlobeBuilder.ConvertPolygonToPsql(polygon)

    # Make polygon parameter required.
    if not postgis_polygon:
      return

    # Get column names of gepoi table and prepare for query.
    table_name = "gepoi_%d" % poi_id
    columns = self.TableColumns("gepoi", table_name)
    # TODO: no results - consider to return 204 No content!?
    if not columns:
      return

    query_cols = ",".join(columns)
    query_cols = query_cols.replace("the_geom", "ST_AsText(the_geom)")

    # Issue SQL query to get search data.
    query = """SELECT %s FROM %s
               WHERE ST_CONTAINS (ST_PolygonFromText(%%s, 4326), the_geom)"""
    query %= (query_cols, table_name)
    results = self.Query("gepoi", query, (postgis_polygon,))

    # TODO: no results - consider to return 204 No content!?
    if results:
      self.PrintTable(columns, results)

  def ListPoiIds(self, target):
    """Lists ids of tables for POI search."""
    # db_id's do NOT match across the gesearch and gestream databases.
    # TODO: Since starting in 5.0 we are reuniting POI search
    #                  with its source database (i.e. one target),
    #                  consider more unification within Postgres.
    # E.g. You should at least be able to get the reference from the
    #      bridging table like:
    # SELECT db_id
    #   FROM target_table t, target_db_table td
    #   WHERE
    #     t.target_path = '/%s' AND
    #     t.target_id = td.target_id
    # Then getting the poi ids would simply be:
    # SELECT poi_id FROM db_poi_table
    #   WHERE db_id = %s"""
    #
    # But unifying the tables so that single-step joins can be done
    # is probably better given our simple schema structure.
    query = """SELECT host_name, db_name
                 FROM target_db_table td, db_table d, target_table t
                 WHERE t.target_path = %s AND
                   t.target_id = td.target_id AND
                   td.db_id = d.db_id
            """
    result = self.Query("gestream", query, (target,))
    if result:
      (db_host, db_name) = result[0]
      query = """SELECT poi_id
                   FROM db_poi_table dp, db_table d
                   WHERE
                     d.host_name = %s AND
                     d.db_name = %s AND
                     d.db_id = dp.db_id
              """
      poi_id = self.Query("gesearch", query, (db_host, db_name))
    else:
      poi_id = []

    print " ".join(map(str, poi_id))

  def PackageGlobeForDownload(self, make_copy, is_map=False):
    """Packages globe or map as a single-file globe."""
    if is_map:
      self.Status("Packaging map for download ...")
      is_2d_str = "--is_2d"
      out_file = self.map_file
    else:
      self.Status("Packaging globe for download ...")
      is_2d_str = ""
      out_file = self.globe_file

    # Remove old globe or map.
    try:
      os.remove(out_file)
    except OSError:
      pass  # Globe or map may not exist.

    make_copy_str = ""
    if make_copy:
      make_copy_str = "--make_copy"

    os_cmd = ("%s/geportableglobepacker --globe_directory=\"%s\" "
              "--output=\"%s\" %s %s"
              % (COMMAND_DIR, self.globe_dir, out_file,
                 make_copy_str, is_2d_str))

    new_globe_size = common.utils.DirectorySize(self.globe_env_dir)
    globe_dir_space = common.utils.DiskSpace(os.path.dirname(out_file))
    if globe_dir_space < new_globe_size:
      self.StatusWarning(
          ("Not enough room to create %s. %s required."
           "<br>Did not execute:<br>%s")
          % (out_file, common.utils.SizeAsString(new_globe_size),
             os_cmd))
      raise DiskFullError("Disk is full at %s"
                          % os.path.dirname(out_file))

    common.utils.ExecuteCmd(os_cmd, self.logger)

    os_cmd = ("chmod a+r \"%s\"" % out_file)
    common.utils.ExecuteCmd(os_cmd, self.logger)
    self.Status("%s %s" % (out_file,
                           common.utils.FileSizeAsString(out_file)))

  def CleanUp(self, save_temp):
    """Clean up temporary directory."""
    try:
      shutil.rmtree(self.globe_final_env_dir)
    except OSError:
      pass  # Directory may not exist

    try:
      if save_temp:
        shutil.move(self.globe_env_dir, self.globe_final_env_dir)
        self.Status("Saving tmp directory as: %s" % self.globe_final_env_dir)
      else:
        shutil.rmtree(self.globe_env_dir)
        self.Status("Deleting tmp directory as: %s" % self.globe_env_dir)
    except Exception, e:
      self.StatusWarning("Error: %s" % str(e))

  def CutProcesses(self):
    """Returns processes referencing the temp directory of a cut in progress."""
    print "ps -ef | grep \"%s\" | grep -v grep" % self.globe_dir
    procs = os.popen("ps -ef | grep \"%s\" | grep -v grep" % self.globe_dir)
    procs_info = []
    for proc in procs:
      proc_info = re.compile(r"\s+").split(proc)
      procs_info.append([int(proc_info[1]), " ".join(proc_info[7:])])
    return procs_info

  def CancelCut(self, save_temp):
    """Kill processes referencing the temp directory of a cut in progress."""
    for proc_info in self.CutProcesses():
      print "Killing: (%d) %s " % (proc_info[0], proc_info[1])
      os.kill(proc_info[0], 1)
    self.CleanUp(save_temp)

  def GetServers(self):
    """Get names and urls for globes being served on this EarthServer."""

    os_cmd = ("%s/geserveradmin --listvss") % COMMAND_DIR
    vss_regex = re.compile("\d+\.\s*(.*?),\s+(.*?),\s+(.*?)\s*$")
    fp = os.popen(os_cmd)
    servers = {}
    for line in fp:
      match = vss_regex.match(line)
      if match and match.group(2) == "ge":
        servers[match.group(1)] = [match.group(3), "false"]
      elif match and match.group(2) == "map":
        servers[match.group(1)] = [match.group(3), "true"]
    fp.close()

    os_cmd = ("%s/geserveradmin --publisheddbs") % COMMAND_DIR
    vss_regex = re.compile("\s*Virtual\s+Server:\s*(.*)$")
    fp = os.popen(os_cmd)
    print "["
    server_entries = []
    for line in fp:
      match = vss_regex.match(line)
      if match and match.group(1) in servers.keys():
        server = "{"
        server += '"name": "%s", ' % match.group(1)
        server += '"url": "%s", ' % servers[match.group(1)][0]
        server += '"is_2d": %s' % servers[match.group(1)][1]
        server += "}"
        server_entries.append(server)
    print ",\n".join(server_entries)
    print "]"
    fp.close()

  def GetDirectorySize(self):
    """Get size of directory for globe being built."""
    size = common.utils.DirectorySizeAsString(self.globe_env_dir)
    if size == "0.00MB":
      return ""
    else:
      return size

  def CheckArgs(self, arg_list, form_):
    """Checks that required arguments are available from form.

    Also sets up all of the the global paths based on the
    globe name.

    Args:
      arg_list: Arguments pass from the GET or POST call.
      form_: HTML form from which the arguments came.
    """
    missing = ""
    uid = form_.getvalue("uid")

    for arg in arg_list:
      if arg == "source":
        value = form_.getvalue_url(arg)
      elif arg == "polygon":
        value = form_.getvalue_kml(arg)
      elif arg == "globe_name":
        value = form_.getvalue_filename(arg)
      else:
        value = form_.getvalue(arg)

      if not value:
        missing = "%s %s" % (missing, arg)
      elif arg == "globe_name":
        self.dbroot_file = DBROOT_FILE_TEMPLATE % (value, uid)
        self.polygon_file = POLYGON_FILE_TEMPLATE % (value, uid, value)
        self.qtnodes_file = QTNODES_FILE_TEMPLATE % (value, uid)
        self.packet_info_file = PACKET_INFO_TEMPLATE % (value, uid)
        self.globe_env_dir = GLOBE_ENV_DIR_TEMPLATE % (value, uid)
        self.globe_final_env_dir = GLOBE_FINAL_ENV_DIR_TEMPLATE % value
        self.globe_dir = GLOBE_DIR_TEMPLATE % (value, uid, value)
        self.icons_dir = ICONS_DIR_TEMPLATE % (value, uid, value)
        self.plugin_dir = PLUGIN_DIR_TEMPLATE % (value, uid, value)
        self.maps_dir = MAPS_DIR_TEMPLATE % (value, uid, value)
        self.json_earth_file = JSON_EARTH_FILE_TEMPLATE % (value, uid, value)
        self.json_map_file = JSON_MAP_FILE_TEMPLATE % (value, uid, value)
        self.info_file = INFO_FILE_TEMPLATE % (value, uid, value)
        self.js_dir = JS_DIR_TEMPLATE % (value, uid, value)
        self.kml_map_file = KML_MAP_FILE_TEMPLATE % (value, uid)
        self.kml_dir = KML_DIR_TEMPLATE % (value, uid, value)
        self.icons_dir = ICONS_DIR_TEMPLATE % (value, uid, value)
        self.search_dir = SEARCH_DIR_TEMPLATE % (value, uid, value)
        self.globe_file = GLOBE_FILE_TEMPLATE % value
        self.map_file = MAP_FILE_TEMPLATE % value
        self.metadata_file = METADATA_FILE_TEMPLATE % (value, uid, value)
        self.logger = common.utils.Log(LOG_FILE % (value, uid))

        form_keys = form_.keys()
        if PORTABLE_PREFIX_PARAM in form_keys:
          self.portable_prefix = form_.getvalue(PORTABLE_PREFIX_PARAM)
        else:
          self.portable_prefix = DEFAULT_PORTABLE_PREFIX

        if PORTABLE_SERVER_PARAM in form_keys:
          self.portable_server = form_.getvalue(PORTABLE_SERVER_PARAM)
        else:
          self.portable_server = DEFAULT_PORTABLE_SERVER

        if PORTABLE_PORT_PARAM in form_keys:
          self.portable_port = form_.getvalue(PORTABLE_PORT_PARAM)
        else:
          self.portable_port = DEFAULT_PORTABLE_PORT

        if SOURCE_GLOBE_PARAM in form_keys:
          self.source_globe = form_.getvalue_path(SOURCE_GLOBE_PARAM)
        else:
          self.source_globe = DEFAULT_SOURCE_GLOBE

        if SEARCH_SERVICE_PARAM in form_keys:
          self.search_service = form_.getvalue(SEARCH_SERVICE_PARAM)
        else:
          self.search_service = DEFAULT_SEARCH_SERVICE

        if KML_SERVER_PARAM in form_keys:
          self.kml_server = form_.getvalue(KML_SERVER_PARAM)
        else:
          self.kml_server = self.portable_server

        if KML_PORT_PARAM in form_keys:
          self.kml_port = form_.getvalue(KML_PORT_PARAM)
        else:
          self.kml_port = self.portable_port

        self.html_address = "%s://%s:%s" % (self.portable_prefix,
                                            self.portable_server,
                                            self.portable_port)
        self.json_address = "%s://%s:%s" % (self.portable_prefix,
                                            self.portable_server,
                                            self.portable_port)
        self.dbroot_dir = DBROOT_DIR_TEMPLATE % (value, uid, value)
        self.dbroot_file2 = DBROOT_FILE2_TEMPLATE % (value, uid, value,
                                                     self.portable_server,
                                                     self.portable_port)

    if missing:
      self.StatusWarning("Missing args: %s" % missing)
      raise


if __name__ == "__main__":
  common.utils.WriteHeader("text/plain")

  msg = "Ok"
  globe_builder = GlobeBuilder()

  # Add directory where globe will be built.
  try:
    # Put the GET arguments into a dictionary.
    cgi_cmd = FORM.getvalue("cmd")

    if cgi_cmd == "ADD_GLOBE_DIRECTORY":
      globe_builder.CheckArgs(["globe_name"], FORM)
      globe_builder.AddGlobeDirectory(FORM.getvalue_escaped("description"))

    elif cgi_cmd == "POLYGON_TO_QTNODES":
      is_mercator = FORM.getvalue("is_mercator")
      globe_builder.CheckArgs(["globe_name", "polygon_level", "polygon"], FORM)
      globe_builder.SavePolygon(FORM.getvalue_kml("polygon"))
      globe_builder.ConvertPolygonToQtNodes(
          int(FORM.getvalue("polygon_level")), is_mercator == "t")

    elif cgi_cmd == "REWRITE_DB_ROOT":
      globe_builder.CheckArgs(["globe_name", "source"], FORM)
      include_historic = FORM.getvalue("include_historical_imagery") is not None
      globe_builder.RewriteDbRoot(FORM.getvalue_url("source"), include_historic)

    elif cgi_cmd == "GRAB_KML":
      globe_builder.CheckArgs(["globe_name", "source"], FORM)
      globe_builder.GrabKml(FORM.getvalue_url("source"))

    elif cgi_cmd == "BUILD_GLOBE":
      ignore_imagery_depth = (FORM.getvalue("ignore_imagery_depth") is not None)
      globe_builder.CheckArgs(["globe_name", "source", "default_level",
                               "max_level"], FORM)
      is_2d = FORM.getvalue("is_2d")
      if is_2d == "t":
        globe_builder.BuildMap(FORM.getvalue_url("source"),
                               int(FORM.getvalue("default_level")),
                               int(FORM.getvalue("max_level")),
                               ignore_imagery_depth)
      else:
        globe_builder.BuildGlobe(FORM.getvalue_url("source"),
                                 int(FORM.getvalue("default_level")),
                                 int(FORM.getvalue("max_level")))

    elif cgi_cmd == "EXTRACT_SEARCH_DB":
      globe_builder.CheckArgs(["globe_name", "source", "polygon"], FORM)
      globe_builder.BuildSearchDb(FORM.getvalue_url("source"),
                                  FORM.getvalue_kml("polygon"))

    elif cgi_cmd == "POI_IDS":
      target_path = common.utils.NormalizeTargetPath(
          FORM.getvalue_url("target"))
      globe_builder.ListPoiIds(target_path)
      msg = ""

    elif cgi_cmd == "SEARCH_FILE":
      globe_builder.CheckArgs(["poi_id", "polygon"], FORM)
      # Note: a value 0 for poi_id is invalid.
      poi_id_arg = int(FORM.getvalue("poi_id", 0))
      polygon_arg = FORM.getvalue_kml("polygon")

      # TODO: return error 400 "Bad request" on else.
      if poi_id_arg and polygon_arg:
        globe_builder.BuildSearchFile(poi_id_arg, polygon_arg)

      msg = ""

    elif cgi_cmd == "ADD_PLUGIN_FILES":
      is_2d = FORM.getvalue("is_2d")
      globe_builder.CheckArgs(["globe_name", "source"], FORM)
      globe_builder.AddPluginFiles(FORM.getvalue_url("source") , is_2d)

    elif cgi_cmd == "PACKAGE_GLOBE":
      globe_builder.CheckArgs(["globe_name"], FORM)
      is_2d = FORM.getvalue("is_2d")
      globe_builder.PackageGlobeForDownload(FORM.getvalue("save_tmp") == "t",
                                            is_2d == "t")

    elif cgi_cmd == "CLEAN_UP":
      globe_builder.CheckArgs(["globe_name"], FORM)
      globe_builder.CleanUp(FORM.getvalue("save_tmp") == "t")

    elif cgi_cmd == "CANCEL":
      globe_builder.CheckArgs(["globe_name", "uid"], FORM)
      globe_builder.CancelCut(FORM.getvalue("save_tmp") == "t")

    elif cgi_cmd == "GLOBE_INFO":
      globe_builder.CheckArgs(["globe_name"], FORM)
      globe_name = FORM.getvalue_filename("globe_name")
      is_2d = FORM.getvalue("is_2d")
      if is_2d == "t":
        print ("<hr>Your map is available at <a href=\"%s/%s.glm\">%s</a>." %
               (WEB_URL_BASE, globe_name, globe_name))
        globe_size = common.utils.FileSizeAsString(globe_builder.map_file)
      else:
        print ("<hr>Your globe is available at <a href=\"%s/%s.glb\">%s</a>." %
               (WEB_URL_BASE, globe_name, globe_name))
        globe_size = common.utils.FileSizeAsString(globe_builder.globe_file)
      print "<br> Size: %s" % globe_size
      globe_builder.AppendInfoFile(globe_size)
      msg = ""

    elif cgi_cmd == "UID":
      allow_overwrite = (FORM.getvalue("allow_overwrite") is not None)
      overwriting = False
      globe_name = FORM.getvalue_filename("globe_name")
      new_globe_name = globe_name
      is_2d = FORM.getvalue("is_2d")
      if is_2d == "t":
        file_template = MAP_FILE_TEMPLATE
      else:
        file_template = GLOBE_FILE_TEMPLATE
      globe_file = file_template % globe_name
      if os.path.isfile(globe_file):
        # If overwriting is allowed, just give a warning.
        if allow_overwrite:
          overwriting = True

        # Otherwise, find an unused name and reserve it.
        else:
          file_num = 1
          while os.path.isfile(globe_file):
            new_globe_name = GLOBE_NAME_TEMPLATE % (globe_name,
                                                    "%03d" % file_num)
            globe_file = file_template % new_globe_name
            file_num += 1

          fp_temp = open(globe_file, "w")
          fp_temp.close()

      msg = "%s %s %s" % (common.utils.Uid(), new_globe_name, overwriting)

    elif cgi_cmd == "SERVERS":
      globe_builder.GetServers()
      msg = ""

    elif cgi_cmd == "PING":
      print "Ready to cut."

    elif cgi_cmd == "BUILD_SIZE":
      globe_builder.CheckArgs(["globe_name"], FORM)
      msg = globe_builder.GetDirectorySize()

    elif cgi_cmd == "BUILD_DONE":
      globe_builder.CheckArgs(["globe_name"], FORM)

      msg = ""
      is_2d = FORM.getvalue("is_2d")
      tool_name = (
          "geportablemapbuilder" if is_2d == "t" else "geportableglobebuilder")

      # To add polling for other command, look for them here
      # as is done for "geportableglobebuilder."
      # Then add a "wait_for_task" in the sequence after the
      # command in globe_cutter.js.
      if common.utils.IsProcessRunningForGlobe(
          tool_name, globe_builder.globe_env_dir):
        msg = "t"
        time.sleep(int(FORM.getvalue("delay", 0)))
      else:
        packet_file = open(globe_builder.packet_info_file)
        packet_info = packet_file.readline().split(" ")
        if len(packet_info) == 3 and int(packet_info[0]) > 0:
          globe_builder.logger.Log("%s imagery packets "
                                   "%s terrain packets "
                                   "%s vector packets "
                                   % tuple(packet_info))
          msg = " ".join(packet_info)
        else:
          globe_builder.logger.Log("FAILED: %s did"
                                   " not complete properly." % tool_name)
          msg = "FAILED %s %s" % (globe_builder.packet_info_file,
                                  packet_info.__str__())

    elif cgi_cmd == "ECHO":
      print FORM.getvalue("echo")

    else:
      GlobeBuilder.StatusWarning("FAILED: Unknown command: %s" % cgi_cmd)
      msg = ""
  except OsCommandError:
    GlobeBuilder.StatusWarning("FAILED: Unable to run OS command")
    msg = ""
  except Exception as e:
    GlobeBuilder.StatusWarning("FAILED: %s" % e)
    msg = ""
  except:
    GlobeBuilder.StatusWarning("FAILED: %s" % sys.exc_info().__str__())
    msg = ""

  if (cgi_cmd != "SERVERS" and cgi_cmd != "PING" and cgi_cmd != "ECHO"
      and cgi_cmd != "BUILD_SIZE" and cgi_cmd != "BUILD_DONE"
      and cgi_cmd != "UID"):
    disk_space = common.utils.DiskSpace(BASE_DIR)
    if disk_space < DISK_SPACE_WARNING_THRESHOLD:
      GlobeBuilder.StatusWarning(
          "WARNING: %4.1f MB of disk space remaining." % disk_space)

  print msg
