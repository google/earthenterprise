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


"""Module implementing core cutting functionality."""

import json
import ssl
import urllib2

from core import search_tab_template


class GlobeCutter(object):
  """Class implementing core cutting functionality."""

  POI_SEARCH_SERVICE_URL = "POISearch"
  PLACES_SEARCH_SERVICE_URL = "/gesearch/PlacesSearch"
  SEARCH_TAB_LIST = "searchTabs : \n[%s\n]"

  @staticmethod
  def GetSearchTabs(source):
    """Get search tabs from the server.

    Args:
      source: database URL (server).
    Returns:
      search tabs in json formatted string or empty string.
    """
    search_tabs = ""
    url = "%s/search_json" % source
    try:
      fp = urllib2.urlopen(url, context=ssl._create_unverified_context())
      if fp.getcode() == 200:
          search_tabs = fp.read()
      fp.close()
    except:
      print ("No search tabs found.")

    return search_tabs

  @staticmethod
  def GetSearchTabByUrl(search_tabs, url):
    """Gets search tab by URL from search tab list.

    Args:
      search_tabs: search tab list.
      url: service URL to look for search tab.
    Returns:
      None or found search tab.
    """
    result_search_tab = None
    for search_tab in search_tabs:
      if search_tab["service_url"] == url:
        result_search_tab = search_tab

    return result_search_tab

  @staticmethod
  def AddSearchTabsToServerDefs(server_defs_in, search_tabs_in):
    """Inserts search tabs into ServerDefs.

    Note: in search json it is expecting POISearch definition as:
    { ...
      "service_url": "POISearch",
      "additional_query_param": "DbId=2&...",
      ...
    }
    and Places search definition as:
    { ...
      "service_url": "/gesearch/PlacesSearch",
      "additional_query_param": "...",
      ...
    }

    Args:
      server_defs_in: ServerDefs in jsonp formatted string.
      search_tabs_in: Search tabs in json formatted string.
    Returns:
      updated ServerDefs merged with search tabs in jsonp formatted string,
      or None if there is no search tabs to add.
    """
    # Detect whether POI and Places search are present in source database.
    search_tabs = json.loads(search_tabs_in)
    poi_search_tab = GlobeCutter.GetSearchTabByUrl(
        search_tabs, GlobeCutter.POI_SEARCH_SERVICE_URL)

    search_tabs_out = []
    # Build POI search tab if source database has POI Search tab.
    if poi_search_tab:
      poi_search_tab_out = search_tab_template.FormatPoiSearchTab(
          poi_search_tab)
      if poi_search_tab_out:
        search_tabs_out.append(poi_search_tab_out)

    # Build GEPlaces search tab.
    places_search_tab_out = search_tab_template.FormatGePlacesSearchTab()
    if places_search_tab_out:
      search_tabs_out.append(places_search_tab_out)

    if not search_tabs_out:
      return None

    search_tabs_out_str = GlobeCutter.SEARCH_TAB_LIST % ", ".join(
        search_tabs_out)

    # Insert search tabs into ServerDefs.
    server_defs_obj_end = server_defs_in.rfind("}")

    server_defs_out = "%s,\n%s%s" % (
        server_defs_in[:server_defs_obj_end],
        search_tabs_out_str,
        server_defs_in[server_defs_obj_end:])

    return server_defs_out


def main():
  pass


if __name__ == "__main__":
  main()
