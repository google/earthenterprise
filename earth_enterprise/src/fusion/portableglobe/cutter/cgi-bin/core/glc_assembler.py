#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
# Copyright 2019, Open GEE Contributors
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

import json
import os
import re
import shutil
import sys
import subprocess
import traceback
import time
import urllib
import yaml
from common import utils
import common.configs
import defusedxml.ElementTree as etree
import xml.etree.ElementTree as etree2

CONFIG_FILE = "/opt/google/gehttpd/cgi-bin/advanced_cutter.cfg"
CONFIGS = common.configs.Configs(CONFIG_FILE)
COMMAND_DIR = CONFIGS.GetStr("COMMAND_DIR")

PORTABLE_TMP_PARAM = "FORCE_PORTABLE_TMP"
GLC_NAME_TEMPLATE = "%s_%s"

TAG_REGEX = re.compile(r"^[A-Za-z_]+:")
LAYER_REGEX = re.compile(r"^layer:")

GLX_ENTRY_REGEX = re.compile(r"^\d+:\s+(.*\.gl[bm])")
POI_SEARCH_ENTRY_REGEX = re.compile(r"^\d+:\s+(.*search_db/.*)")

LAYERS_REGEX = re.compile(r"layers\s+:")
ENTRY_REGEX = re.compile(
    r"\s*[\"\']?(.+?)[\"\']?\s*:\s*[\"\']?(.*?)[\"\']?\,?\s*$")

TEMPLATE_DIR = r"/opt/google/gehttpd/htdocs/cutter/template"

DRY_RUN = False
BASE_DIR = "%s/glc_assembly/glc_%s"
SEARCH_DIR = "%s/search_db"
KML_DIR = "%s/kml"
EARTH_DATA_DIR = "%s/earth"
MAPS_DATA_DIR = "%s/maps"
REL_EARTH_DATA_DIR = "earth"
REL_MAPS_DATA_DIR = "maps"
REL_INFO_FILE = "earth/info.txt"
REL_INFO_FILE_LOG = "log/info.txt"
REL_POLYGON_FILE = "earth/polygon.kml"
REL_META_DBROOT_FILE = "metadbroot"
REL_MAP_JSON_FILE = "map.json"
REL_LAYERS_INFO_FILE = "earth/layer_info.txt"
REL_DBROOT_LAYERS_INFO_FILE = "earth/dbroot_layer_info.txt"
REL_DELTA_LAYERS_INFO_FILE = "earth/delta_layer_info.txt"
INFO_FILE = "%%s/%s" % REL_INFO_FILE
INFO_FILE_LOG = "%%s/%s" % REL_INFO_FILE_LOG
META_DBROOT_FILE = "%%s/%s" % REL_META_DBROOT_FILE
LAYERS_INFO_FILE = "%%s/%s" % REL_LAYERS_INFO_FILE
DBROOT_LAYERS_INFO_FILE = "%%s/%s" % REL_DBROOT_LAYERS_INFO_FILE
POLYGON_FILE = "%%s/%s" % REL_POLYGON_FILE
DELTA_LAYERS_INFO_FILE = "%%s/%s" % REL_DELTA_LAYERS_INFO_FILE
TEMP_GLC_FILE = "%s/temp.glc"
EXTRACT_MAP_JSON_FILE = "%%s/%s" % REL_MAP_JSON_FILE
MAP_JSON_FILE = "%%s/maps/%s" % REL_MAP_JSON_FILE


DOWNLOAD_TEMPLATE = "<a href=\"/cutter/globes/%s.glc\">Download %s</a>"

GLM_STARTING_INDEX = 200

GEE_SERVER_DEFS = """var geeServerDefs = {
isAuthenticated : false,
layers : [
%s
],
projection : "%s",
serverUrl : "http://localhost:9335"
};"""

GEE_LAYER_DEF = """
    {
      icon : "icons/773_l.png",
      id : %d,
      glm_id : %d,
      initialState : %s,
      isPng : true,
      label : "%s",
      lookAt : "none",
      opacity : 1,
      requestType : "%s",%s
      version : 1
    }
"""

PRODUCTS_DIR = "%s/globes" % CONFIGS.GetStr("DATA_DIR")


class OsCommandError(Exception):
  """Thrown if os command fails."""
  pass


class DiskFullError(Exception):
  """Thrown if disk partition is too full."""
  pass


class GlcAssembler(object):
  """Class that implements all commands for cutting a globe."""

  def __init__(self):
      self.info_file = ""

  @staticmethod
  def Status(message):
    """Outputs a status message."""
    print "\n%s" % message

  @staticmethod
  def GetUniqueName(file_template, glc_name, allow_overwrite):
    """Find a unique name for the glc."""
    overwriting = False
    glc_file = file_template % glc_name
    new_glc_name = glc_name
    if os.path.isfile(glc_file):
      # If overwriting is allowed, just give a warning.
      if allow_overwrite:
        overwriting = True

      # Otherwise, find an unused name and reserve it.
      else:
        file_num = 1
        while os.path.isfile(glc_file):
          new_glc_name = GLC_NAME_TEMPLATE % (glc_name, "%03d" % file_num)
          glc_file = file_template % new_glc_name
          file_num += 1

        fp_temp = open(glc_file, "w")
        fp_temp.close()
    return (new_glc_name, overwriting)

  def PrepareAssembler(self, base_path):
    """Set up build directory and paths to be used by assembler."""
    self.base_path = base_path
    self.maps_data_dir = MAPS_DATA_DIR % self.base_path
    self.earth_data_dir = EARTH_DATA_DIR % self.base_path
    self.search_dir = SEARCH_DIR % self.base_path
    self.kml_dir = KML_DIR % self.base_path
    self.polygon_file = POLYGON_FILE % self.base_path
    self.meta_dbroot_file = META_DBROOT_FILE % self.base_path
    self.map_json_file = MAP_JSON_FILE % self.base_path
    self.extract_map_json_file = EXTRACT_MAP_JSON_FILE % self.base_path
    self.layers_info_file = LAYERS_INFO_FILE % self.base_path
    self.dbroot_layers_info_file = DBROOT_LAYERS_INFO_FILE % self.base_path
    self.info_file = INFO_FILE % self.base_path
    self.temp_glc = TEMP_GLC_FILE % self.base_path

    logger = self.GetLogger()
    utils.PrintAndLog("Prepare assembler...", logger)
    utils.PrintAndLog("Base dir: %s" % self.base_path, logger)
    utils.CreateDirectory(self.base_path)
    utils.CopyDirectory("%s/earth" % TEMPLATE_DIR, self.earth_data_dir, logger)
    utils.CopyDirectory("%s/maps" % TEMPLATE_DIR, self.maps_data_dir, logger)
    utils.CreateDirectory(self.search_dir)
    utils.CreateDirectory(self.kml_dir)
    utils.PrintAndLog("Prepare assembler done.", logger)
    return logger

  def GetLogger(self):
    """Get logger built from info.txt."""

    if not self.info_file:
      # self.info_file is expected to be set before calling this method
      return None

    if not os.path.exists(os.path.dirname(self.info_file)):
      os.makedirs(os.path.dirname(self.info_file))
    logger = utils.Log(self.info_file)
    return logger

  def ExtractFileFromGlx(self, glc_path, file_in_glc, output_file):
    """Extract given file from glc file and save it to the given path."""
    os_cmd = ("%s/geglxinfo "
              "--glx=\"%s\" "
              "--extract_file=\"%s\" "
              "--output=\"%s\""
              % (COMMAND_DIR, glc_path, file_in_glc, output_file))
    result = utils.RunCmd(os_cmd)
    if not os.path.isfile(output_file):
      return "FAILED: Unable to create {0}\n{1!r}".format(output_file, result)
    else:
      return "Extracting %s %s\n" % (
          output_file, utils.FileSizeAsString(output_file))

  def ExtractFilelistFromGlx(self, glc_path, regex):
    """Extract given file from glc file and save it to the given path."""
    logger = self.GetLogger()
    os_cmd = ("%s/geglxinfo "
              "--glx=\"%s\" "
              "--list_files"
              % (COMMAND_DIR, glc_path))
    glx_info = utils.RunCmd(os_cmd)

    # Checking for errors and logging them if there where any
    if not glx_info[0] and len(glx_info) == 2:
      utils.PrintAndLog("ERROR: '%s' running command: '%s'" % (glx_info[1], os_cmd), logger)
      return []

    glx_files = []
    for line in glx_info:
      match = regex.match(line)
      if match:
        glx_files.append(match.groups()[0])
    return glx_files

  def CreateMetaDbRoot(self, base_glb_info, logger):
    """Create the metadbroot for a 3d glc."""
    # Extra check to prevent XSS attack.
    if self.IsPathInvalid({"base": self.base_path,
                           "dbroot": os.path.dirname(self.meta_dbroot_file),
                           "layer_info": self.dbroot_layers_info_file,
                          }):
      return ""

    os.chdir(self.base_path)
    os_cmd = ("%s/gecreatemetadbroot "
              "--output=\"%s\" "
              "--layers=\"%s\" "
              % (COMMAND_DIR,
                 self.meta_dbroot_file,
                 self.dbroot_layers_info_file))
    if base_glb_info:
      if base_glb_info["has_base_imagery"]:
        os_cmd += " --has_base_imagery"
      if base_glb_info["has_proto_base_imagery"]:
        os_cmd += " --has_proto_imagery"
      if base_glb_info["has_base_terrain"]:
        os_cmd += " --has_terrain"
    utils.ExecuteCmd(os_cmd, logger)

  def ExtractLayerInfo(self):
    """Extract layer info from json (geeserverdefs)."""
    fp = open(self.extract_map_json_file, "r")
    found_layers = False
    layers = []
    for line in fp:
      if not found_layers and LAYERS_REGEX.match(line):
        found_layers = True
      else:
        if "[" in line:
          # Start reading layer array.
          pass
        elif "]" in line:
          return layers
        elif "{" in line:
          layer = {}
        elif "}" in line:
          layers.append(layer)
          layer = {}
        else:
          match = ENTRY_REGEX.match(line)
          if match:
            groups = match.groups()
            layer[groups[0]] = groups[1]
    return layers

  def GetJsonLayers(self, path):
    """Extract glm json and then extract layer info from it."""
    self.ExtractFileFromGlx(
        path, "maps/map.json", self.extract_map_json_file)
    return self.ExtractLayerInfo()

  def GetJson(self, path):
    """Extract <map.json> from a GLM, and return it parsed."""
    subprocess.check_call([
      os.path.join(COMMAND_DIR, "geglxinfo"),
      "--glx={0}".format(path),
      "--extract_file={0}".format("maps/map.json"),
      "--output={0}".format(self.extract_map_json_file)
    ])

    with open(self.extract_map_json_file, 'r') as f:
      # Skip "var geeServerDefs =" to get to the JSON:
      while f.read(1) != '=':
        continue
      json_string = f.read()
    # Skip the last ";" to leave just the JSON:
    json_string = json_string[0 : json_string.rfind(';')]

    # Fusion does not currently produce valid JSON for geeServerDefs, so we
    # (mis)use a YAML parser instead:
    return yaml.load(json_string)

  def CreateMapJson(self, index, layers_info, projection):
    """Create the glc JSON (geeServerDefs) from that of the glms."""
    layer_defs = []
    for layer_info in layers_info:
      layer_defs.append(GEE_LAYER_DEF % (
          int(layer_info["channel"]),
          index + layer_info["glmIndex"],
          layer_info["initiallyVisible"].__str__().lower(),
          layer_info["name"],
          layer_info["requestType"],
          ""))

    fp = open(self.map_json_file, "w")
    fp.write(GEE_SERVER_DEFS % (",".join(layer_defs), projection))
    fp.close()

  def LayerInfoFiles(self, index, layer_type, layers_info):
    """Build content for layerinfo.txt, extracting search files if needed."""
    layer_info_content = ""
    search_idx = 1
    kml_idx = 1
    for layer_info in layers_info:
      try:
        layer_info_content += "%s %s %s %d 0\n" % (
            layer_info["name"], layer_info["path"], layer_type, index)
        if layer_info["use_search"]:
          search_files = self.ExtractFilelistFromGlx(
              layer_info["path"], POI_SEARCH_ENTRY_REGEX)
          for search_file in search_files:
            relative_address = "search_db/gepoi_%d" % search_idx
            search_idx += 1
            layer_info_content += "info %s FILE 0 0\n" % relative_address
            output_file = "%s/%s" % (self.base_path, relative_address)
            self.ExtractFileFromGlx(
                layer_info["path"], search_file, output_file)
            print "Writing %s to %s ..." % (search_file, output_file)
      except KeyError:
        if layer_info["grab_kml"]:
          try:
            # Grab kml content.
            fp = urllib.urlopen(layer_info["url"])
            content = fp.read()
            fp.close()

            # Write it in glc kml directory.
            kml_file = "kml_%03d.kml" % kml_idx
            kml_idx += 1
            kml_path = "%s/%s" % (self.kml_dir, kml_file)
            fp = open(kml_path, "w")
            fp.write(content)
            fp.close()

            # Add it to the layer_info file.
            layer_info_content += "%s kml/%s FILE 0 0\n" % (kml_file, kml_file)

            # Modify url to point at local copy in glc.
            layer_info["url"] = "/kml/%s" % kml_file
            print "Downloaded to %s and made it available at %s." % (
                kml_path, layer_info["url"])
          except IOError:
            print "Unable to write kml."

        else:
          # Ok, skip kml entries.
          print "Skipping kml: %s" % layer_info["url"]

      index += 1

    layer_info_content += "info %s FILE 0 0\n" % REL_INFO_FILE

    for path in os.listdir(self.earth_data_dir):
      layer_info_content += "%s %s/%s FILE 0 0\n" % (
          path, REL_EARTH_DATA_DIR, path)

    for path in os.listdir(self.maps_data_dir):
      layer_info_content += ("%s %s/%s FILE 0 0\n" % (
          path, REL_MAPS_DATA_DIR, path))

    layer_info_content += "layerInfo %s FILE 0 0\n" %  REL_LAYERS_INFO_FILE

    return layer_info_content

  def DbrootLayerInfo(self, index, layer_type, layers_info):
    """Build content for layerinfo.txt, extracting search files if needed."""
    dbroot_info_content = ""
    for layer_info in layers_info:
      try:
        dbroot_info_content += "%s %s %s %d 0\n" % (
            layer_info["name"], layer_info["path"], layer_type, index)
      except KeyError:
        dbroot_info_content += "%s %s KML %d 0\n" % (
            layer_info["name"], layer_info["url"], index)
      index += 1
    return dbroot_info_content

  def Create3dLayerInfoFile(self, layers_info, base_glb_info):
    """Build 3d layerinfo.txt file."""
    layer_info_content = ""
    dbroot_layer_info_content = ""
    layer_index = 1
    if (base_glb_info and
        "base_glb_path" in base_glb_info.keys() and
        base_glb_info["base_glb_path"]):
      layer_info_content += "%s %s %s %d 0\n" % (
          "base_globe", base_glb_info["base_glb_path"], "IMAGE", layer_index)
      dbroot_layer_info_content = layer_info_content

    layer_index += 1
    layer_info_files = self.LayerInfoFiles(layer_index, "IMAGE", layers_info)
    layer_info_content = "%s%s" % (layer_info_content, layer_info_files)

    layer_info_content += "%s %s %s 0 0\n" % (
        "dbRoot", REL_META_DBROOT_FILE, "DBROOT")

    utils.CreateFile(self.layers_info_file, layer_info_content)

    dbroot_layer_info_content += (self.DbrootLayerInfo(layer_index,
                                                       "IMAGE",
                                                       layers_info))

    utils.CreateFile(self.dbroot_layers_info_file, dbroot_layer_info_content)

  def Create2dLayerInfoFile(self, layers_info, glms, projection):
    """Build 2d layerinfo.txt file."""
    layer_info_content = self.LayerInfoFiles(
        GLM_STARTING_INDEX, "MAP", glms)
    self.CreateMapJson(GLM_STARTING_INDEX, layers_info, projection)
    layer_info_content += "map.json maps/map.json FILE 0 0\n"
    utils.CreateFile(self.layers_info_file, layer_info_content)

  def Assemble2dGlcFiles(self, layers_info, glms, logger):
    """Assemble glms into a glc."""
    first_glm_json = self.GetJson(glms[0]['path'])

    # Verify all GLMs have the same projection:
    warned_about_projections = False
    for i in xrange(1, len(glms)):
      glm_json = self.GetJson(glms[i]['path'])
      if glm_json['projection'] != first_glm_json['projection']:
        if not warned_about_projections:
          utils.PrintAndLog(
            """<span class="warning">WARNING: GLMs with different projections, clients may display mismatched layers.</span>""",
            logger, None)
          utils.PrintAndLog(
            "  First file {0}, projection: {1}".format(glms[0]['path'], first_glm_json['projection']),
            logger, None)
          warned_about_projections = True
        utils.PrintAndLog(
          "  File {0}, projection: {1}".format(glms[i]['path'], glm_json['projection']),
          logger, None)

    self.Create2dLayerInfoFile(layers_info, glms, first_glm_json['projection'])

  def Assemble3dGlcFiles(self, layers_info, base_glb_info, logger):
    """Assemble glbs into a glc."""
    self.Create3dLayerInfoFile(layers_info, base_glb_info)
    self.CreateMetaDbRoot(base_glb_info, logger)

  def DeleteGlc(self, path):
    if os.path.exists(path):
      print "\nDeleting %s ..." % path
      os.remove(path)

  # Example 3d json request:
  # {
  #   "layers":[
  #     {
  #       "name":"--_layer_1_--",
  #       "path":"-- path to layer 1 glb --",
  #       "use_search":true
  #     },
  #     {
  #       "name":"--_layer_2_--",
  #       "path":"-- path to layer 2 glb --",
  #       "use_search":false,
  #     },
  #     {
  #       "name":"--_kml_layer_--",
  #       "url":"-- kml url only --",
  #       "grab_kml":true
  #     }
  #   ],
  #   "description":"-- the description --",
  #   "polygon":"-- the polygon --",
  #   "is_2d":false,
  #   "base_glb": {
  #     "base_glb_path":"-- the base glb path --",
  #     "has_base_imagery":true,
  #     "has_proto_base_imagery":false,
  #     "has_base_terrain":true
  #   }
  # }
  #
  # Example 2d json request:
  # {
  #   "glms":[
  #     {
  #       "name":"-- glm 1 --",
  #       "path":"-- glm 1 path --",
  #       "use_search":true
  #     },
  #     {
  #       "name":"-- glm 2 --",
  #       "path":"-- glm 2 path --",
  #       "use_search":false
  #     }
  #   ],
  #   "layers":[
  #     {
  #       "glmIndex":0,
  #       "name":"-- layer 1 --",
  #       "channel":"1004",
  #       "requestType" : "ImageryMaps",
  #       "initiallyVisible" : true,
  #       "isOpaque" : true
  #     },
  #     {
  #       "glmIndex":1,
  #       "name":"-- layer 2 --",
  #       "channel":"1000",
  #       "requestType" : "ImageryMaps",
  #       "initiallyVisible" : true,
  #       "isOpaque" : true
  #     },
  #     {
  #       "glmIndex":0,
  #       "name":"-- layer 3 --",
  #       "channel":"1002",
  #       "requestType" : "VectorMapsRaster",
  #       "initiallyVisible" : true,
  #       "isOpaque" : false
  #     }
  #   ],
  #   "description":"-- description --",
  #   "polygon":"-- polygon --"
  #   "is_2d":true
  # }

  def WritePolygonFile(self, polygon, logger):
    with open(self.polygon_file, "w") as fp:
      # Check XML validity and standardize representation
      utils.PrintAndLog("Checking polygon")
      xml = etree2.ElementTree(etree.fromstring(str(polygon)))
      utils.PrintAndLog("Writing polygon")
      xml.write(fp, xml_declaration=True, encoding='UTF-8')
      utils.PrintAndLog("SUCCESS", logger, None)

  def AssembleGlc(self, form_):
    """Assemble a 2d or 3d glc based on given parameters."""
    utils.PrintAndLog("Assembling ...")
    try:
      spec_json = form_.getvalue_json("glc_assembly_spec")
      spec = json.loads(spec_json)
    except Exception as e:
      utils.PrintAndLog("Failed to parse glc assembly json:" + str(e))
      raise e

    msg = ""
    try:
      # Prepare (create and copy) directories.
      base = form_.getvalue_path("base")
      name = form_.getvalue_filename("name")
      path = form_.getvalue_path("path")
      overwriting = form_.getvalue("overwrite")
      logger = self.PrepareAssembler(base)

      # Extra check to prevent XSS attack.
      if self.IsPathInvalid({"base": base,
                             "path": os.path.dirname(path),
                            }):
        return ""

      if overwriting:
        utils.PrintAndLog("*** Overwriting %s ***" % name, logger)
        try:
          self.DeleteGlc(path)
          utils.PrintAndLog("SUCCESS", logger, None)
        except IOError:
          utils.PrintAndLog("FAILED", logger, None)
          raise

      # Create file to reserve name.
      utils.PrintAndLog("Reserve file: %s" % path, logger)
      fp = open(path, "w")
      fp.close()
      utils.PrintAndLog("SUCCESS", logger, None)

      layers_info = spec["layers"]
      if spec["is_2d"]:
        try:
          glms = spec["glms"]
        except KeyError:
          glms = None
      else:
        try:
          base_glb_info = spec["base_glb"]
        except KeyError:
          base_glb_info = None

      utils.PrintAndLog("Create info file: %s" % self.info_file, logger)
      os.chdir(self.base_path)
      utils.CreateInfoFile(self.info_file,
                           utils.HtmlEscape(spec["description"]))
      utils.PrintAndLog("SUCCESS", logger, None)

      utils.PrintAndLog("Create polygon file: %s" % self.polygon_file, logger)

      self.WritePolygonFile(spec["polygon"], logger)

      if spec["is_2d"]:
        utils.PrintAndLog("Building 2d glc at %s ..." % path, logger)
        self.Assemble2dGlcFiles(layers_info, glms, logger)
      else:
        utils.PrintAndLog("Building 3d glc at %s ..." % path, logger)
        self.Assemble3dGlcFiles(layers_info, base_glb_info, logger)

      # --make_copy is needed because we are using another volume.
      os_cmd = ("%s/geportableglcpacker "
                "--layer_info=\"%s\" "
                "--output=\"%s\" --make_copy"
                % (COMMAND_DIR, self.layers_info_file, self.temp_glc))
      utils.ExecuteCmdInBackground(os_cmd, logger)

    except OsCommandError as e:
      utils.PrintAndLog("Error: Unable to run OS command.", logger)
      msg = "FAILED"
    except:
      utils.PrintAndLog("Exception: {0}".format(traceback.format_exc()))
      msg = "FAILED"
    return msg

  def GetJobInfo(self, form_):
    """Get job information (base, name, path, and etc) as json."""
    tmp_dir = form_.getvalue_path(PORTABLE_TMP_PARAM)
    if not tmp_dir:
      tmp_dir = CONFIGS.GetStr("TMP_DIR")
    base = BASE_DIR % (tmp_dir, utils.Uid())
    name = form_.getvalue_filename("name")
    allow_overwrite = (form_.getvalue("allow_overwrite") is not None)
    file_template = "%s/%%s.glc" % PRODUCTS_DIR
    name_info = GlcAssembler.GetUniqueName(
        file_template, name, allow_overwrite)
    globe_name = name_info[0]
    overwriting = name_info[1]
    globe_path = "%s/%s.glc" % (PRODUCTS_DIR, globe_name)

    msg = {
        "base": base,
        "uid": base.split("/")[-1],
        "name": globe_name,
        "path": globe_path,
        "overwrite": overwriting,
    }
    return json.dumps(msg, sort_keys=True, indent=4, separators=(",", ": "))

  def IsDone(self, tool_name, form_):
    """Check whether a job (with given name and base folder) is done or not.

    Args:
      tool_name: tool name to check if it is present in list of running
                 processes.
      form_: form object to get parameters submitted by client.
    Returns:
      status of job: {"RUNNING", "SUCCEEDED", "FAILED"}.
    """
    base = form_.getvalue_path("base")
    # Extra check to prevent XSS attack.
    if self.IsPathInvalid({"base": base}):
      return ""

    delay = int(form_.getvalue("delay", 0))

    if utils.IsProcessRunningForGlobe(tool_name, base):
      time.sleep(delay)
      return "RUNNING"

    if os.path.exists(TEMP_GLC_FILE % base):
      return "SUCCEEDED"
    else:
      return "FAILED"

  def GetSize(self, form_):
    """Get the file size of temp glc."""
    base = form_.getvalue_path("base")
    temp_glc = TEMP_GLC_FILE % base
    if os.path.exists(temp_glc):
      globe_size = common.utils.FileSizeAsString(temp_glc)
    else:
      globe_size = "0.00MB"
    return globe_size

  def CleanUp(self, form_):
    """Move temp glc to final location and remove temp folder if needed."""
    self.base_path = form_.getvalue_path("base")
    self.info_file = INFO_FILE % self.base_path
    temp_glc = TEMP_GLC_FILE % self.base_path
    final_glc = form_.getvalue_path("globe")
    cleanup = form_.getvalue("cleanup") is not None
    logger = self.GetLogger()
    utils.PrintAndLog("Moving temp glc %s -> %s ..." % (temp_glc, final_glc),
                      logger)
    shutil.move(temp_glc, final_glc)
    utils.PrintAndLog("SUCCESS", logger, None)

    if cleanup:
      utils.PrintAndLog("Deleting %s" % self.base_path, logger)
      shutil.rmtree(self.base_path)
      utils.PrintAndLog("SUCCESS", logger, None)

    return ""

  def GetGlobeInfo(self, form_):
    """Get information of final glc (link, filesize)."""
    globe_name = form_.getvalue_filename("name")
    download_link = DOWNLOAD_TEMPLATE % (globe_name, globe_name)
    msg = "Glc is available at %s\n" % download_link
    path = "%s/%s.glc" % (PRODUCTS_DIR, globe_name)
    globe_size = common.utils.FileSizeAsString(path)
    msg += "Size: %s" % globe_size
    return msg

  def IsPathInvalid(self, paths):
    """Prevent XSS attack by double-checking paths are real file paths."""
    for path in paths:
      if not os.path.exists(paths[path]):
        utils.PrintAndLog("ERROR: invalid \'%s\': %s" % (path, paths[path]))
        return True
    return False

  def PrepareDisassembler(self, base_path, glc_path):
    """Set up build directory and paths to be used by disassembler."""
    self.base_path = base_path
    self.output_dir = base_path
    self.glc_path = glc_path
    self.info_file = INFO_FILE_LOG % self.base_path

    logger = self.GetLogger()
    utils.PrintAndLog("Prepare disassembler...", logger)
    utils.PrintAndLog("Output dir: %s" % self.base_path, logger)
    utils.PrintAndLog("Glc path: %s" % self.glc_path, logger)

    utils.PrintAndLog("Prepare disassembler done.", logger)
    return logger

  def DisassembleGlc(self, form_):
    """Extract glms or glbs from a glc."""

    logger = None

    try:
      output_dir = form_.getvalue_path("dir")
      glc_path = form_.getvalue_path("path")
      logger = self.PrepareDisassembler(output_dir, glc_path)

      # Extra check to prevent XSS attack.
      if self.IsPathInvalid({"glc_path": self.glc_path,
                             "output_dir": self.output_dir,
                            }):
        return "FAILED: Output dir or Glc path is invalid"

      glx_entries = self.ExtractFilelistFromGlx(self.glc_path, GLX_ENTRY_REGEX)
      msg = ""
      if len(glx_entries) == 0:
        return "FAILED: no file entries found in '%s' that match pattern '%s'" % (self.glc_path, GLX_ENTRY_REGEX.pattern)
      for glx_entry in glx_entries:
        glx_name = glx_entry.split("/")[-1]
        extracted_glx = "%s/%s" % (self.output_dir, glx_name)
        if os.path.isfile(extracted_glx):
          return msg + "FAILED: %s already exists." % extracted_glx

        print "Extracting %s to %s ..." % (glx_entry, extracted_glx)
        msg += self.ExtractFileFromGlx(self.glc_path, glx_entry, extracted_glx)
      return "SUCCESS"
    except Exception as e:
      exc_type, exc_value, exc_traceback = sys.exc_info()
      err_msg = e.__str__()
      stack_trace = repr(traceback.format_exception(exc_type, exc_value, exc_traceback))
      if logger is not None:
        logger.Log("ERROR: %s\n%s\n" % (err_msg, stack_trace))
      else:
        print "ERROR: %s\n%s\n" % (err_msg, stack_trace)
      return "FAILED %s" % err_msg


def main():
  pass

if __name__ == "__main__":
  main()
