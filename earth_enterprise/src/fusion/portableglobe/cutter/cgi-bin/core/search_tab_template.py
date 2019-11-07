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

"""Module defines Search Tabs template strings."""

# TODO: remove displayKeys-parameter from POI search url
# after unifying displayKeys-usage in portable vs GEE search services.

# POI search tab template.
POI_SEARCH_TAB = """
{
args :
[
{
screenLabel : "%s",
urlTerm : "%s"
}
]
,
tabLabel : "POI Search",
url : "/Portable2dPoiSearch?service=PoiPlugin%s"
}
"""

# Places search tab template.
GEPLACES_SEARCH_TAB = """
{
args :
[
{
screenLabel : "%s",
urlTerm : "%s"
}
]
,
tabLabel : "GEPlaces",
url : "/Portable2dPoiSearch?service=GEPlacesPlugin%s"
}
"""


def FormatPoiSearchTab(source_tab):
  """Formats POI search tab based on a template.

  Args:
    source_tab: original POI search tab definition.
  Returns:
    GEPlaces search tab denition for portable or None.
  """
  if not source_tab["fields"]:
    return None

  fields_0 = source_tab["fields"][0]

  # Get a screenLabel.
  screen_label = fields_0["label"] if fields_0["label"] else ""

  # Get a urlTerm.
  # TODO: consider to get search term value from source search tab,
  # i.e. url_term = fields_0["key"].  In this case, we have to unify
  # search plugins on GEE and Portable servers.
  url_term = "searchTerm"

  # Get query parameters.
  query_params = "&displayKeys=%s" % url_term

# Note: not required for portable search plugin.
#  additional_query_param = source_tab["additional_query_param"]
#  if additional_query_param:
#    query_params = "%s&%s" % (query_params, additional_query_param)

#  additional_config_param = source_tab["additional_config_param"]
#  if additional_config_param:
#    query_params = "%s&%s" % (query_params, additional_config_param)

  # Format search tab definition.
  poi_search_tab = POI_SEARCH_TAB % (screen_label, url_term, query_params)

  return poi_search_tab


def FormatGePlacesSearchTab():
  """Formats GEPlaces search tab based on a template.

  Returns:
    GEPlaces search tab denition for portable.
  """
  # TODO: consider to get screen label and search term values from
  # source Places search tab, i.e. screen_label = fields_0["label"],
  # url_term = fields_0["key"].
  # In this case, we have to unify corresponding search plugins on GEE and
  # Portable servers.

  # Set screen label.
  screen_label = "Places"

  # Set search term.
  url_term = "location"

  # Get query parameters.
  query_params = "&displayKeys=%s" % url_term

  return GEPLACES_SEARCH_TAB % (screen_label, url_term, query_params)


def main():
  pass


if __name__ == "__main__":
  main()
