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



"""Example search service that uses a general db file of populated locations.

This service provides:
  A) a function that registers the service.
  B) a search service object that inherits from this class with 2 methods:
    1) KmlSearch: a method for getting kml search results.
    2) JsonSearch: a method for getting json search results.

Along with methods from the base class for creating the kml and json, it uses
the working directory, that has been set via a base class method call, for
locating the geplaces db file.
"""

import os
import re

import ge_base_search

# These should match what you used when defining the search tab.
SEARCH_SERVICE_NAME = "GEPlacesPlugin"
SEARCH_SERVICE_KEY = "location"
# Alternate search key for Google Earth 6.2.2+.
ALT_SEARCH_SERVICE_KEY = "q"
DATA_STORE_NAME = "GEPlaces Search Results"


class GEPlacesSearch(ge_base_search.GEBaseSearch):
  """GEPlaces search service for finding populated locations."""

  def __init__(self):
    # Initialize the base class.
    super(GEPlacesSearch, self).__init__()

  def Search(self, handler, placemark_fn, search_term,
             delimiter="", max_results=20):
    """Does a grep-based search on geplaces using given placemark renderer.

    Since the geplaces file is stored in rough order by population, the
    results will come back with most populous locations first.

    Args:
      handler: Web server handler serving this request.
      placemark_fn: Function to call to render the next placemark.
      search_term: Search term (or phrase) to use in grep.
      delimiter: Delimiter to use between placemarks.
      max_results: Maximum number of results (placemarks) to create.
    """
    if self.LatLngSearch(handler, placemark_fn, search_term):
      return

    database_file = "%s%sgeplaces%splaces_200.txt" % (
        self.working_directory_, os.sep, os.sep)
    try:
      fp = open(database_file)
      fp.close()
    except IOError:
      print "Unable to find: %s" % database_file

    cnt = 0
    pattern = re.compile(search_term, re.IGNORECASE)
    for func in (pattern.match, pattern.search):
      fp = open(database_file)
      for line in fp:
        if func(line):
          data = line.split("|:")
          if cnt > 0:
            if delimiter:
              handler.write(delimiter)

          placemark_fn(handler, data[0], "", "", data[0], data[4],
                       "Population: %s<br>Latitude: %s<br>Longitude: %s"
                       % (data[5], data[2], data[3]),
                       data[3], data[2])
          cnt += 1
          if cnt >= max_results:
            break
      fp.close()
      if cnt > 0:
        break

  def KmlSearch(self, handler):
    """Does the indicated search and returns the results as KML.

    Args:
      handler: Web server handler serving this request.
    """
    try:
      search_term = handler.request.arguments[SEARCH_SERVICE_KEY][0]
    except KeyError:
      # Earth 6.2.2+ will use "q" instead.
      search_term = handler.request.arguments[ALT_SEARCH_SERVICE_KEY][0]
    self.KmlStart(handler, search_term)
    self.Search(handler, self.KmlPlacemark, search_term)
    self.KmlEnd(handler)

  def JsonSearch(self, handler, cb):
    """Does the indicated search and return the results as JSON.

    Args:
      handler: Web server handler serving this request.
      cb: Json callback variable name.
    """
    search_term = handler.request.arguments[SEARCH_SERVICE_KEY][0]
    self.JsonStart(handler, cb, DATA_STORE_NAME, search_term)
    self.Search(handler, self.JsonPlacemark, search_term, ",")
    self.JsonEnd(handler)


def RegisterSearchService(search_services):
  """Creates a new search service object and adds it by name to the dict.

  Args:
    search_services: dict to which the new search service should be added
        using its name as the key.
  Returns:
    the new search service object.
  """
  if SEARCH_SERVICE_NAME in search_services.keys():
    print "Warning: replacing existing %s service." % SEARCH_SERVICE_NAME

  search_services[SEARCH_SERVICE_NAME] = GEPlacesSearch()
  return search_services[SEARCH_SERVICE_NAME]
