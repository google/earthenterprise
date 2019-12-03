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

"""Module for implementing the Google search."""

import urllib2
from search.common import utils


class SearchGoogle(object):
  """Class for performing the Google geocoder search.

  Search will be delegated to the Google backend services
  based on the output format(KML or JSON).
  """

  EARTH_GEOCODE_URL = "https://www.google.com:443/earth/rpc/search"
  MAPS_GEOCODE_URL = "https://maps.googleapis.com:443/maps/api/geocode/json"

  def __init__(self):
    """Inits SearchGoogle.

    Initializes the logger "ge_search".
    """
    self.utils = utils.SearchUtils()
    self.logger = self.utils.logger

  def HandleSearchRequest(self, environ):
    """Prepares the Google Geocoder URL and performs the google geocoder search.

    Args:
     environ: A dictionary object containing CGI/WSGI style
       environment variables.
    Returns:
     search_results: A KML/JSONP formatted string which contains search results.
    """
    search_results = ""
    response_type = self.utils.GetResponseType(environ)

    query_string = environ["QUERY_STRING"]
    user_agent = environ["HTTP_USER_AGENT"]
    if response_type == "KML":
      http_url = "%s?%s" % (SearchGoogle.EARTH_GEOCODE_URL, query_string)
    else:
      http_url = "%s?%s" % (SearchGoogle.MAPS_GEOCODE_URL,
                            query_string.replace("q=", "address=", 1))

    req = urllib2.Request(http_url)
    req.add_header('User-Agent', user_agent)
    self.logger.debug("Search Google URL: %s", http_url)
    search_results = urllib2.urlopen(req).read()

    # TODO: Use try.. except block and
    # send appropriate error based on the return code/reason, as below
    #  try:
    #    search_results = urllib2.urlopen(url).read()
    #  except urllib2.URLError as e:
    #    if hasattr(e, 'reason'):
    #      ...
    #    elif hasattr(e, 'code'):
    #      ...

    return (search_results, response_type)


def main():
  pass

if __name__ == "__main__":
  main()
