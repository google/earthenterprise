#!/usr/bin/env python
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


"""Module that implements all commands for cutting a globe."""

import os
import re
import shutil
import urllib
import common.utils

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

# Disk space minimum in MB before we start sending warnings.
DISK_SPACE_WARNING_THRESHOLD = 1000.0


class OsCommandError(Exception):
  """Thrown if os command fails."""
  pass


class DiskFullError(Exception):
  """Thrown if disk partition is too full."""
  pass


class GlobeBuilder(object):
  """Class that implements all commands for cutting a globe."""

  @staticmethod
  def Query(query, db):
    """Submits the query to the database and returns tuples.

    Args:
      query: SQL SELECT statement.
      db: The database being queried.
    Returns:
      Results as list of lists (rows of fields).
    """
    separator = "=^="
    os_cmd = ("%s%spsql -U geuser -c \"%s\" -P format=unaligned "
              "-P tuples_only -P fieldsep=\"%s\" %s") % (
                  COMMAND_DIR, os.sep, query, separator, db)
    out = os.popen(os_cmd)
    results = []
    for line in out:
      row = line.strip().split(separator)
      if len(row) == 1:
        results.append(row[0])
      else:
        results.append(row)
    out.close()
    return results

  @staticmethod
  def TableColumns(table, db):
    """Returns list of column names for the given table in the given db."""
    query = ("SELECT column_name FROM INFORMATION_SCHEMA.columns "
             "WHERE table_name='%s'") % table
    return GlobeBuilder.Query(query, db)

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
    # Convert POINT() data to a lat and low column.
    for row in data:
      if fix_geom:
        point = row[-1]
        lonlat = point[6:-1].split(" ")
        row[-1] = lonlat[0]
        row.append(lonlat[1])
      print "\t".join(row)

  @staticmethod
  def Status(message):
    """Outputs a status message."""
    print "<br>%s" % message

  @staticmethod
  def StatusWarning(message):
    """Outputs a status message."""
    print "<br><span class='text_status_warn'>%s</span>" % message

  def __init__(self, tmp_dir_):
    """Set up cut directory and paths to be used."""
    if not tmp_dir_:
      self.TMP_DIR = WEB_DIR
    else:
      self.TMP_DIR = tmp_dir_

    self.BASE_DIR = "%s/.globe_builder" % self.TMP_DIR
    self.GLOBE_ENV_DIR_TEMPLATE = "%s/%%s_%%s" % self.BASE_DIR
    self.GLOBE_FINAL_ENV_DIR_TEMPLATE = "%s/%%s_env" % self.BASE_DIR
    self.LOG_FILE = "%s/log" % self.GLOBE_ENV_DIR_TEMPLATE
    self.GLOBE_DIR_TEMPLATE = "%s/%%s" % self.GLOBE_ENV_DIR_TEMPLATE
    self.ICONS_DIR_TEMPLATE = "%s/%%s/icons" % self.GLOBE_ENV_DIR_TEMPLATE
    self.PLUGIN_DIR_TEMPLATE = "%s/%%s/earth" % self.GLOBE_ENV_DIR_TEMPLATE
    self.MAPS_DIR_TEMPLATE = "%s/%%s/maps" % self.GLOBE_ENV_DIR_TEMPLATE
    self.JSON_EARTH_FILE_TEMPLATE = (
        "%s/%%s/earth/earth.json" % self.GLOBE_ENV_DIR_TEMPLATE)
    self.JSON_MAP_FILE_TEMPLATE = (
        "%s/%%s/maps/map.json" % self.GLOBE_ENV_DIR_TEMPLATE)
    self.INFO_FILE_TEMPLATE = (
        "%s/%%s/earth/info.txt" % self.GLOBE_ENV_DIR_TEMPLATE)
    self.JS_DIR_TEMPLATE = "%s/%%s/js" % self.GLOBE_ENV_DIR_TEMPLATE
    self.KML_MAP_FILE_TEMPLATE = "%s/kml_map.txt" % self.GLOBE_ENV_DIR_TEMPLATE
    self.KML_DIR_TEMPLATE = "%s/%%s/kml" % self.GLOBE_ENV_DIR_TEMPLATE
    self.SEARCH_DIR_TEMPLATE = "%s/%%s/search_db" % self.GLOBE_ENV_DIR_TEMPLATE
    self.DBROOT_FILE_TEMPLATE = "%s/dbroot.v5" % self.GLOBE_ENV_DIR_TEMPLATE
    self.DBROOT_DIR_TEMPLATE = "%s/%%s/dbroot" % self.GLOBE_ENV_DIR_TEMPLATE
    self.DBROOT_FILE2_TEMPLATE = (
        "%s/%%s/dbroot/dbroot_%%s_%%s" % self.GLOBE_ENV_DIR_TEMPLATE)
    self.POLYGON_FILE_TEMPLATE = (
        "%s/%%s/earth/polygon.kml" % self.GLOBE_ENV_DIR_TEMPLATE)
    self.PACKET_INFO_TEMPLATE = (
        "%s/packet_info.txt" % self.GLOBE_ENV_DIR_TEMPLATE)
    self.QTNODES_FILE_TEMPLATE = "%s/qt_nodes.txt" % self.GLOBE_ENV_DIR_TEMPLATE

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
    fp = open(self.polygon_file, "w")
    if polygon:
      fp.write(polygon)
    fp.close()
    self.Status("Saved polygon to %s" % self.polygon_file)

  def ConvertPolygonToQtNodes(self, polygon_level, is_mercator=False):
    """Convert polygon into a set of qt nodes at given level."""
    self.Status("Convert polygon to quadtree nodes ...")
    try:
      os.remove(self.qtnodes_file)
    except OSError:
      pass  # Ok, if file isn't there.

    os_cmd = ("%s/gepolygontoqtnodes --qt_nodes_file=%s "
              "--kml_polygon_file=%s --max_level=%d"
              % (COMMAND_DIR, self.qtnodes_file, self.polygon_file,
                 polygon_level))
    if is_mercator:
      os_cmd += " --mercator"

    self.ExecuteCmd(os_cmd)
    fp = open(self.qtnodes_file)
    self.Status("%d qtnodes" % len(fp.readlines()))
    fp.close()

  @staticmethod
  def ConvertPolygonToPsql(polygon):
    """Convert polygon kml into PostGIS function for sql."""
    if not polygon:
      return ""

    parsed_polygon = polygon[polygon.find("<Polygon>"):
                             polygon.find("</Polygon>")].strip()
    parsed_polygon = (parsed_polygon[parsed_polygon.find("<coordinates>")
                                     + len("<coordinates>"):
                                     parsed_polygon.find("</coordinates>")])
    parsed_polygon = parsed_polygon.strip()
    coordinates = parsed_polygon.split(" ")
    num_coordinates = len(coordinates) - 1
    if num_coordinates < 3:
      return ""

    first_point = coordinates[0].split(",")
    postgis_polygon = "ST_PolygonFromText('POLYGON((%s %s" % (first_point[0],
                                                              first_point[1])
    for i in xrange(num_coordinates):
      next_point = coordinates[i + 1].split(",")
      postgis_polygon = "%s, %s %s" % (postgis_polygon,
                                       next_point[0], next_point[1])
    postgis_polygon = "%s%s\n" % (postgis_polygon, "))', 4326)")

    return postgis_polygon

  def RewriteDbRoot(self, source):
    """Executes command to rewrite the dbroot and extract the icons it uses."""
    self.Status("Rewrite dbroot ...")
    os_cmd = ("%s/gerewritedbroot --source=%s --icon_directory=%s "
              "--dbroot_file=%s --search_service=%s "
              "--kml_map_file=%s"
              % (COMMAND_DIR, source, self.icons_dir, self.dbroot_file,
                 self.search_service, self.kml_map_file))
    self.ExecuteCmd(os_cmd)
    self.Status("%d icons" % len(os.listdir(self.icons_dir)))

    os_cmd = ("cp %s %s"
              % (self.dbroot_file, self.dbroot_file2))
    self.ExecuteCmd(os_cmd)

  def GrabKml(self, source):
    """Recursively grabs all kml files referenced in the dbroot."""
    self.Status("Grab kml files ...")
    os_cmd = (("%s/gekmlgrabber --kml_map_file=%s --output_directory=%s"
               " --source=%s --kml_server=%s --kml_port=%s")
              % (COMMAND_DIR, self.kml_map_file, self.kml_dir,
                 source, self.kml_server, self.kml_port))
    self.ExecuteCmd(os_cmd)
    self.Status("%d kml files" % len(os.listdir(self.kml_dir)))

  def BuildGlobe(self, source, default_level, max_level):
    """Executes command to cut globe and save data into packet bundles."""
    self.Status("Build globe ...")
    # Run this task as a background task.
    os_cmd = ("%s/geportableglobebuilder --source=%s --default_level=%d "
              "--max_level=%d --hires_qt_nodes_file=%s "
              "--globe_directory=%s --dbroot_file=%s > %s &"
              % (COMMAND_DIR, source, default_level, max_level,
                 self.qtnodes_file, self.globe_dir, self.dbroot_file,
                 self.packet_info_file))
    self.ExecuteCmd(os_cmd)

  def BuildMap(self, source, default_level, max_level):
    """Executes command to cut map and save data into packet bundles."""
    self.Status("Build map ...")
    # Run this task as a background task.
    # Having trouble with permissions if output is redirected to a file.
    os_cmd = ("%s/geportablemapbuilder --source=%s --hires_qt_nodes_file=%s "
              "--map_directory=%s  --default_level=%d --max_level=%d &"
              % (COMMAND_DIR, source, self.qtnodes_file,
                 self.globe_dir, default_level, max_level))
    self.ExecuteCmd(os_cmd)

    # TODO: Get real packet numbers for imagery and vectors.
    fp = open(self.packet_info_file, "w")
    fp.write("1 0 0")
    fp.close()

  def CreateInfoFile(self):
    """Create globe info file."""
    fp = open(self.info_file, "w")
    fp.write("Portable Globe\n")
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

  def AddJsonFile(self, source, portable_server, json_file):
    """Get JSON from server and add it to the portable globe plugin files."""
    # Get JSON from the server.
    url = "%s/query?request=Json&var=geeServerDefs" % source
    self.Status("Rewrite JSON from: %s to: %s" % (url, json_file))
    fp = urllib.urlopen(url)
    json = fp.read()
    fp.close()

    # Replace all urls to point at the portable server.
    start = 0
    new_json = ""
    is_altered = False
    for match in re.finditer(r"([\w\-]+)\s*:\s*\"http[s]?:"
                             r"//[\w\-\.]+(:\d+)?/([^/\n]+)([^\n\"]*)\"",
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
    for match in re.finditer(r"icon\s*:\s*\"icons/(.*)\"",
                             json, 0):
      icons[match.groups()[0]] = True

    for icon in icons.iterkeys():
      # Get JSON from the server.
      url = "%s/query?request=Icon&icon_path=icons/%s" % (source, icon)
      try:
        fp = urllib.urlopen(url)
        fpw = open("%s/%s" % (self.icons_dir, icon), "w")
        fpw.write(fp.read())
        fp.close()
        fpw.close()
      except Exception:
        self.Status("Unable to retrieve icon %s" % icon)

  def AddPluginFiles(self, source, is_map):
    """Copies files associated with the Google Earth browser plug-in."""
    self.Status("Add plugin files ...")
    # Plugin directory should already exist.
    os_cmd = "cp -r %s/earth/* %s" % (TEMPLATE_DIR, self.plugin_dir)
    self.ExecuteCmd(os_cmd)

    try:
      os.makedirs(self.maps_dir)
    except os.error:
      pass  # Directory may already exist

    os_cmd = "cp -r %s/maps/* %s" % (TEMPLATE_DIR, self.maps_dir)
    self.ExecuteCmd(os_cmd)

    try:
      os.makedirs(self.js_dir)
    except os.error:
      pass  # Directory may already exist

    os_cmd = "cp -r %s/js/* %s" % (TEMPLATE_DIR, self.js_dir)
    self.ExecuteCmd(os_cmd)

    # Get the Json that defines the plugin params.
    if is_map:
      json = self.AddJsonFile(source, self.json_address, self.json_map_file)
      self.AddJsonIcons(source, json)
    else:
      self.AddJsonFile(source, self.json_address, self.json_earth_file)

  def BuildSearchDb(self, source, polygon):
    """Extracts database info needed for POI search."""
    self.Status("Extract search data ...")
    try:
      os.makedirs(self.search_dir)
    except os.error:
      pass  # Directory may already exist

    # Determine the server and fusion_db from the source.
    fusion_db = ""
    server = "http://localhost/"
    if source:
      while source[-1:] == "/":
        source = source[:-1]

      server_idx = 0
      if source[:7] == "http://":
        server_idx = 7
      elif source[:8] == "https://":
        server_idx = 8

      idx = source.find("/", server_idx)
      if idx > 0:
        server = source[:idx]
        fusion_db = source[idx+1:]
      else:
        server = source

    base_url = "%s/cgi-bin/globe_cutter.pyo" % server

    url = "%s?cmd=POI_IDS&target=%s" % (base_url, fusion_db)
    self.Status("Getting search poi ids: %s" % url)
    fp = urllib.urlopen(url)
    poi_list = fp.read().strip()
    fp.close()
    if poi_list:
      poi_ids = poi_list.split(" ")
      for i in xrange(len(poi_ids)):
        url = ("%s?cmd=SEARCH_FILE&poi_id=%s&polygon=%s"
               % (base_url, poi_ids[i], urllib.quote(polygon)))
        self.Status("Getting search poi data: %s" % url)
        search_file = "%s/gepoi_%s" % (self.search_dir, poi_ids[i])
        self.Status("Saving search poi data: %s" % search_file)
        try:
          fp = urllib.urlopen(url)
          fpw = open(search_file, "w")
          fpw.write(fp.read().strip())
          fpw.write("\n")
          fp.close()
          fpw.close()
        except IOError:
          print "Unable to write search file: %s" % search_file
    else:
      self.Status("No search data.")

  def BuildSearchFile(self, poi_id, postgis_polygon):
    """Extracts database info needed for POI search."""
    sql = ("SELECT %s FROM gepoi_%s WHERE ST_CONTAINS" +
           "(%s, the_geom)" % postgis_polygon)
    table = "gepoi_%s" % poi_id
    columns = self.TableColumns(table, "gepoi")
    query_cols = ",".join(columns)
    query_cols = query_cols.replace("the_geom", "ST_AsText(the_geom)")
    query = sql % (query_cols, poi_id)
    results = self.Query(query, "gepoi")
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
                 WHERE
                   (t.target_path = '%s' OR
                    t.target_path = '/%s') AND
                   t.target_id = td.target_id AND
                   td.db_id = d.db_id
            """ % (target, target)
    result = self.Query(query, "gestream")
    if result[0]:
      (db_host, db_name) = result[0]
      query = """SELECT poi_id
                   FROM db_poi_table dp, db_table d
                   WHERE
                     d.host_name = '%s' AND
                     d.db_name = '%s' AND
                     d.db_id = dp.db_id
              """ %  (db_host, db_name)
      poi_id = self.Query(query, "gesearch")
    else:
      poi_id = []

    print " ".join(poi_id)

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

    os_cmd = ("%s/geportableglobepacker --globe_directory=%s --output=%s %s %s"
              % (COMMAND_DIR, self.globe_dir, out_file, make_copy_str,
                 is_2d_str))

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

    self.ExecuteCmd(os_cmd)

    os_cmd = ("chmod a+r %s" % out_file)
    self.ExecuteCmd(os_cmd)
    self.Status("%s %s" % (out_file,
                           common.utils.FileSizeAsString(out_file)))

  def CleanUp(self, save_temp_):
    """Clean up temporary directory."""
    try:
      shutil.rmtree(self.globe_final_env_dir)
    except OSError:
      pass  # Directory may not exist

    try:
      if save_temp_:
        shutil.move(self.globe_env_dir, self.globe_final_env_dir)
        self.Status("Saving tmp directory as: %s" % self.globe_final_env_dir)
      else:
        shutil.rmtree(self.globe_env_dir)
        self.Status("Deleting tmp directory as: %s" % self.globe_env_dir)
    except Exception, e:
      self.StatusWarning("Error: %s" % str(e))

  def GetServers(self):
    """Get names and urls for globes being served on this EarthServer."""

    os_cmd = ("%s/geserveradmin --listvss") % COMMAND_DIR
    vss_regex = re.compile(r"\d+\.\s*(.*?),\s+(.*?),\s+(.*?)\s*$")
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
    vss_regex = re.compile(r"\s*Virtual\s+Server:\s*(.*)$")
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

  def ExecuteCmd(self, os_cmd):
    """Execute os command and log results."""
    self.Status("Executing: %s" % os_cmd)
    self.AppendInfoFile(os_cmd)
    try:
      os.system(os_cmd)
      self.logger.Log("SUCCESS")
    except Exception, e:
      self.logger.Log("FAILED: %s" % e.__str__())
      raise OsCommandError()

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
      value = form_.getvalue(arg)
      if not value:
        missing = "%s %s" % (missing, arg)
      elif arg == "globe_name":
        self.dbroot_file = self.DBROOT_FILE_TEMPLATE % (value, uid)
        self.polygon_file = self.POLYGON_FILE_TEMPLATE % (value, uid, value)
        self.qtnodes_file = self.QTNODES_FILE_TEMPLATE % (value, uid)
        self.packet_info_file = self.PACKET_INFO_TEMPLATE % (value, uid)
        self.globe_env_dir = self.GLOBE_ENV_DIR_TEMPLATE % (value, uid)
        self.globe_final_env_dir = self.GLOBE_FINAL_ENV_DIR_TEMPLATE % value
        self.globe_dir = self.GLOBE_DIR_TEMPLATE % (value, uid, value)
        self.icons_dir = self.ICONS_DIR_TEMPLATE % (value, uid, value)
        self.plugin_dir = self.PLUGIN_DIR_TEMPLATE % (value, uid, value)
        self.maps_dir = self.MAPS_DIR_TEMPLATE % (value, uid, value)
        self.json_earth_file = self.JSON_EARTH_FILE_TEMPLATE % (
            value, uid, value)
        self.json_map_file = self.JSON_MAP_FILE_TEMPLATE % (value, uid, value)
        self.info_file = self.INFO_FILE_TEMPLATE % (value, uid, value)
        self.js_dir = self.JS_DIR_TEMPLATE % (value, uid, value)
        self.kml_map_file = self.KML_MAP_FILE_TEMPLATE % (value, uid)
        self.kml_dir = self.KML_DIR_TEMPLATE % (value, uid, value)
        self.icons_dir = self.ICONS_DIR_TEMPLATE % (value, uid, value)
        self.search_dir = self.SEARCH_DIR_TEMPLATE % (value, uid, value)
        self.globe_file = self.GLOBE_FILE_TEMPLATE % value
        self.map_file = self.MAP_FILE_TEMPLATE % value
        self.logger = common.utils.Log(self.LOG_FILE % (value, uid))

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
          self.source_globe = form_.getvalue(SOURCE_GLOBE_PARAM)
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
        self.dbroot_dir = self.DBROOT_DIR_TEMPLATE % (value, uid, value)
        self.dbroot_file2 = self.DBROOT_FILE2_TEMPLATE % (value, uid, value,
                                                          self.portable_server,
                                                          self.portable_port)

    if missing:
      self.StatusWarning("Missing args:%s" % missing)
      raise


def main():
  pass

if __name__ == "__main__":
  main()
