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

"""Module for implementing the Federated search."""

from search.common import exceptions
from search.common import utils
from search.plugin import coordinate_search_handler
from search.plugin import geplaces_search_handler


class FederatedSearch(object):
  """Class for performing the Federated search.

  We initially submit the search against the CoordinateSearch, stopping
  there if any positive results are returned. If not, we issue our search
  against the GEPlacesSearch.

  If there is a valid response from any of the searches, we use
  it. If not, then we can assume that 'location' doesn't exist and so we
  present a reasonable 'no results found' back to the caller.
  """

  def __init__(self):
    """Inits FederatedSearch.

    Initializes the logger "ge_search".
    """
    self.utils = utils.SearchUtils()
    self.logger = self.utils.logger

    # Create coordinate and places search objects

    self._coordinate = coordinate_search_handler.CoordinateSearch()
    self._geplaces = geplaces_search_handler.PlacesSearch()

    # Get Style information from Places or Coordinate search handlers.
    self._style = self._geplaces.style

  def HandleSearchRequest(self, environ):
    """Fetches the search tokens from form and performs the federated search.

    Args:
     environ: A list of environment variables as supplied by the
      WSGI interface to the federated search application interface.
    Returns:
     search_results: A KML/JSONP formatted string which contains search results.
     response_type: Response type can be KML or JSONP, depending on the client.
    """
    search_results = ""
    search_status = False

    # Fetch all the attributes provided by the user.
    parameters = self.utils.GetParameters(environ)

    self._geplaces.parameters = parameters
    self._coordinate.parameters = parameters

    # Retrieve the function call back name for JSONP response.
    self.f_callback = self.utils.GetCallback(parameters)

    # Fetch additional query parameters 'flyToFirstElement' and
    # 'displayKeys' from URL.
    self.fly_to_first_element = self.utils.GetValue(
        parameters, "flyToFirstElement")
    self.display_keys_string = self.utils.GetValue(
        parameters, "displayKeys")

    response_type = self.utils.GetResponseType(environ)

    original_query = self.utils.GetValue(parameters, "q")

    if original_query:
      (search_status, search_results) = self.DoSearch(
          original_query, response_type)
    else:
      self.logger.debug("Empty search query received")

    if not search_status:
      folder_name = "No results were returned."
      search_results = self.utils.NoSearchResults(
          folder_name, self._style, response_type, self.f_callback)

    return (search_results, response_type)

  def DoSearch(self, original_query, response_type):
    """Performs the federated search and return's the results.

    Args:
     original_query: search query as entered by the user.
     response_type: Response type can be KML or JSONP, depending on the client.
    Returns:
     tuple containing
     search_status: Whether search could be performed.
     search_results: A KML/JSONP formatted string which contains search results.
    """
    search_status = False
    search_results = ""

    self._geplaces.f_callback = self.f_callback
    self._coordinate.f_callback = self.f_callback

    self._geplaces.fly_to_first_element = self.fly_to_first_element
    self._geplaces.display_keys_string = self.display_keys_string

    self.logger.debug("Performing coordinate search on %s", original_query)

    try:
      (search_status, search_results) = self._coordinate.DoSearch(
          original_query, response_type)
    except exceptions.BadQueryException:
      # If 'BadQueryException' exception occurs, ignore it
      # and proceed with places search.
      pass

    if not search_status:
      self.logger.debug(
          "No search results were returned by coordinate search."
          "Proceeding with places search...")
      (search_status, search_results) = self._geplaces.DoSearch(
          original_query, response_type)
      if not search_status:
        self.logger.debug(
            "No search results were returned by coordinate and places search.")

    return search_status, search_results


def main():
  fedobj = FederatedSearch()
  fedobj.DoSearch("santa clara", "KML")

if __name__ == "__main__":
  main()
