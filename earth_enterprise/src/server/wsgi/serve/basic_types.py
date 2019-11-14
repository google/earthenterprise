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

"""Definitions for basic types using in serving.
"""

import json

from common import string_utils

# Portable files extensions set.
PORTABLE_EXTS_SET = [".glm", ".glb", ".glc"]


class DbFlags(object):
  """Database binary properties that are being packed as bit-fields.

  Constants used for setting/masking corresponding bits in db_table.db_flags
  field in gestream database.
  """
  # The 2D Fusion database uses Google Basemap.
  USE_GOOGLE_BASEMAP = 1


class DbType(object):
  """Database types enum."""
  TYPE_GE = "ge"
  TYPE_MAP = "map"
  TYPE_GLB = "glb"
  TYPE_GLM = "glm"
  TYPE_GLC = "glc"


class SearchTypeCode(object):
  """The search types codes."""
  NO_SEARCH = 0
  FEDERATED = 1
  POI = 2
  SUPPLEMENTAL = 4


class DbInfo(object):
  """Database info used in json response."""

  def __init__(self):
    """Inits DbInfo object."""
    self.name = ""
    # Fusion DB unique identifier is {host:path} pair.
    self.host = ""  # Fusion hostname
    self.path = ""  # Path in assetroot for Fusion database.
    # The name of virtual host which database published for.
    self.virtual_host_name = ""
    self.target_base_url = ""    # Built based on virtual host URL.
    # The target point - relative path.
    self.target_path = ""
    self.type = ""
    self.is_mercator = False  # whether Mercator database/portable.
    self.is_2d = False        # If both is_2d and is_3d are False, it may
    self.is_3d = False        # indicate undefined.
    self.timestamp = None
    self.size = 0
    self.description = ""
    self.has_poi = False     # whether has POI data.
    self.serve_wms = False   # whether wms serving is enabled for the target.
    self.default_database = False # whether this database is published as the default
    # Whether portable has polygon.
    # Set to None since it is only being used for portables, and undefined for
    # Fusion databases.
    self.has_polygon = None
    # Whether registered on Server.
    # Note: All Fusion databases pushed on Server are registered.
    self.registered = False
    # Whether pushed from Fusion on different host.
    self.remote = False

  def DumpJson(self):
    return json.dumps(self, cls=DbInfoJsonEncoder)


class DbInfoJsonEncoder(json.JSONEncoder):
  """Json encoder for DbInfo object."""

  def default(self, obj):
    if not isinstance(obj, DbInfo):
      return json.JSONEncoder.default(self, obj)

    return obj.__dict__


class SearchField(object):
  """Object used for serializing search definitions."""

  def __init__(self, label=None, suggestion=None, key=None):
    """Inits SearchField object."""
    self.label = label
    self.suggestion = suggestion
    self.key = key

  @staticmethod
  def Deserialize(val):
    """Deserializes value as SearchField object.

    Args:
      val: json formatted string or dictionary.
    Returns:
      SearchField object.
    """
    if isinstance(val, str):
      obj = JsonToObj(val)
    else:
      assert isinstance(val, type)
      obj = val

    return SearchField(obj.label, obj.suggestion, obj.key)


class SearchDef(object):
  """Object used for serializing search definitions.

  Object that is being serialized to search.json file. It has an additional
  property 'supplemental_ui_hidden' in compare with a search definition object
  of SearchDefContent-type stored in database.
  """

  def __init__(self, label=None, service_url=None,
               fields=None, additional_query_param=None,
               additional_config_param=None,
               html_transform_url=None,
               kml_transform_url=None,
               suggest_server=None,
               result_type=None,
               supplemental_ui_hidden=False):
    """Inits SearchDef object."""
    self.label = label
    self.service_url = service_url
    self.fields = fields if fields else []
    self.additional_query_param = additional_query_param
    self.additional_config_param = additional_config_param
    self.html_transform_url = (html_transform_url if html_transform_url
                               else "about:blank")
    self.kml_transform_url = (kml_transform_url if kml_transform_url
                              else "about:blank")
    self.suggest_server = suggest_server if suggest_server else "about:blank"
    self.result_type = result_type if result_type else "KML"
    self.supplemental_ui_hidden = supplemental_ui_hidden

  @staticmethod
  def Deserialize(val):
    """Deserializes value as SearchDef object.

    Args:
      val: json formatted string or dictionary.
    Returns:
      SearchDef object.
    """
    if isinstance(val, str):
      obj = JsonToObj(val)
    else:
      assert isinstance(val, type)
      obj = val

    fields = [SearchField.Deserialize(field) for field in obj.fields]

    return SearchDef(
        obj.label, obj.service_url, fields,
        obj.additional_query_param,
        obj.additional_config_param,
        obj.html_transform_url if hasattr(obj, "html_transform_url") else None,
        obj.kml_transform_url if hasattr(obj, "kml_transform_url") else None,
        obj.suggest_server if hasattr(obj, "suggest_server") else None,
        obj.result_type if hasattr(obj, "result_type") else None)

  def DumpJson(self):
    """Serializes object to json formatted string."""

    return json.dumps(self, cls=SearchDefJsonEncoder)


class SearchDefJsonEncoder(json.JSONEncoder):
  """Json encoder for SearchDef object."""

  def default(self, obj):
    if isinstance(obj, SearchDef) or isinstance(obj, SearchField):
      return obj.__dict__

    return json.JSONEncoder.default(self, obj)


class SearchDefContent(object):
  """Object used for serializing search definition content.

  Object that is stored in gesearch database.
  """

  def __init__(self, label=None, service_url=None,
               fields=None, additional_query_param=None,
               additional_config_param=None,
               html_transform_url=None,
               kml_transform_url=None,
               suggest_server=None,
               result_type=None):
    """Inits SearchDef object."""
    self.label = label
    self.service_url = service_url
    self.fields = fields if fields else []
    self.additional_query_param = additional_query_param
    self.additional_config_param = additional_config_param
    self.html_transform_url = (html_transform_url if html_transform_url
                               else "about:blank")
    self.kml_transform_url = (kml_transform_url if kml_transform_url
                              else "about:blank")
    self.suggest_server = suggest_server if suggest_server else "about:blank"
    self.result_type = result_type if result_type else "KML"

  @staticmethod
  def Deserialize(val):
    """Deserializes value as SearchDefContent object.

    Args:
      val: json formatted string or dictionary.
    Returns:
      SearchDefContent object.
    """
    if isinstance(val, str):
      obj = JsonToObj(val)
    else:
      assert isinstance(val, type)
      obj = val

    fields = [SearchField.Deserialize(field) for field in obj.fields]

    return SearchDefContent(
        obj.label, obj.service_url, fields,
        obj.additional_query_param,
        obj.additional_config_param,
        obj.html_transform_url if hasattr(obj, "html_transform_url") else None,
        obj.kml_transform_url if hasattr(obj, "kml_transform_url") else None,
        obj.suggest_server if hasattr(obj, "suggest_server") else None,
        obj.result_type if hasattr(obj, "result_type") else None)

  def DumpJson(self):
    """Serializes object to json formatted string."""

    return json.dumps(self, cls=SearchDefContentJsonEncoder)


class SearchDefContentJsonEncoder(json.JSONEncoder):
  """Json encoder for SearchDefContent object."""

  def default(self, obj):
    if isinstance(obj, SearchDefContent) or isinstance(obj, SearchField):
      return obj.__dict__

    return json.JSONEncoder.default(self, obj)


class SnippetSet(object):
  """SnippetSet object used in json response."""

  def __init__(self, set_name=None, end_snippet=None):
    """Inits object.

    Args:
      set_name: a name of a snippet set.
      end_snippet: a content of a snippet set.
    """
    self.set_name = set_name
    self.end_snippet = end_snippet

  def DumpJson(self):
    """Dumps object to json string."""
    return json.dumps(self, cls=SnippetSetJsonEncoder)


class SnippetSetJsonEncoder(json.JSONEncoder):
  """Json encoder for SnippetSet object."""

  def default(self, obj):
    if not isinstance(obj, SnippetSet):
      return json.JSONEncoder.default(self, obj)

    return obj.__dict__


def JsonToObj(json_str):
  """Gets object from json string.

  e.g. use JsonToObj(s).fields[0].search_term to get the search_term value from
  json string "{..,"fields": [{"search_term": "q", "suggestion": "Country"}]}"

  Note: here, unicode strings that we get from the json loader are converted
  to ASCII, and text sanitizing is performed.

  Args:
    json_str: json string.
  Returns:
    object restored from json string.
  """

  def HToO(x):
    if isinstance(x, dict):
      return type("jo", (), dict((k, HToO(v)) for (k, v) in x.iteritems()))
    elif isinstance(x, list):
      return [HToO(v) for v in x]
    elif isinstance(x, unicode):
      return string_utils.SanitizeText(x.encode("ascii"))
    elif isinstance(x, str):
      return string_utils.SanitizeText(x)
    else:
      return x

  return HToO(json.loads(json_str))


# Note: when adding a new object with a JsonEncoder, please update the class
# GenericJsonEncoder.
class GenericJsonEncoder(json.JSONEncoder):
  """Json encoder for all basic types objects."""

  def default(self, obj):
    if (isinstance(obj, DbInfo) or
        isinstance(obj, SearchDef) or
        isinstance(obj, SearchField) or
        isinstance(obj, SearchDefContent) or
        isinstance(obj, SnippetSet)):
      return obj.__dict__

    return json.JSONEncoder.default(self, obj)

  @staticmethod
  def DumpJson(obj):
    return json.dumps(obj, cls=GenericJsonEncoder)


def main():
  pass


if __name__ == "__main__":
  main()
