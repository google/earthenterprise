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

"""Module for implementing the Example search."""

import cgi
import os
from string import Template
import psycopg2
from psycopg2.pool import ThreadedConnectionPool
from search.common import exceptions
from search.common import geconstants
from search.common import utils


class ExampleSearch(object):
  """Class for performing the Example search.

  Example search is the neighborhood search that demonstrates
  how to construct and query a spatial database based on URL
  search string, extract geometries from the result, associate
  various styles with them and return the response back to the client

  Valid Inputs are:
  q=pacific heights
  neighborhood=pacific heights
  """

  def __init__(self):
    """Inits ExampleSearch.

    Initializes the logger "ge_search".
    Initializes templates for kml, json, placemark templates
    for the KML/JSONP output.
    Initializes parameters for establishing a connection to the database.
    """

    self.utils = utils.SearchUtils()
    constants = geconstants.Constants()

    configs = self.utils.GetConfigs(
        os.path.join(geconstants.SEARCH_CONFIGS_DIR, "ExampleSearch.conf"))

    style_template = self.utils.style_template
    self._jsonp_call = self.utils.jsonp_functioncall
    self._geom = """
            <name>%s</name>
            <styleUrl>%s</styleUrl>
            <Snippet>%s</Snippet>
            <description>%s</description>
            %s\
    """
    self._json_geom = """
         {
            "name": "%s",
            "Snippet": "%s",
            "description": "%s",
            %s
         }\
    """

    self._placemark_template = self.utils.placemark_template
    self._kml_template = self.utils.kml_template

    self._json_template = self.utils.json_template
    self._json_placemark_template = self.utils.json_placemark_template

    self._example_query_template = (
        Template(constants.example_query))

    self.logger = self.utils.logger

    self._user = configs.get("user")
    self._hostname = configs.get("host")
    self._port = configs.get("port")

    self._database = configs.get("databasename")
    if not self._database:
      self._database = constants.defaults.get("example.database")

    self._pool = ThreadedConnectionPool(
        int(configs.get("minimumconnectionpoolsize")),
        int(configs.get("maximumconnectionpoolsize")),
        database=self._database,
        user=self._user,
        host=self._hostname,
        port=int(self._port))

    self._style = style_template.substitute(
        balloonBgColor=configs.get("balloonstyle.bgcolor"),
        balloonTextColor=configs.get("balloonstyle.textcolor"),
        balloonText=configs.get("balloonstyle.text"),
        iconStyleScale=configs.get("iconstyle.scale"),
        iconStyleHref=configs.get("iconstyle.href"),
        lineStyleColor=configs.get("linestyle.color"),
        lineStyleWidth=configs.get("linestyle.width"),
        polyStyleColor=configs.get("polystyle.color"),
        polyStyleColorMode=configs.get("polystyle.colormode"),
        polyStyleFill=configs.get("polystyle.fill"),
        polyStyleOutline=configs.get("polystyle.outline"),
        listStyleHref=configs.get("iconstyle.href"))

  def RunPGSQLQuery(self, query, params):
    """Submits the query to the database and returns tuples.

    Note: variables placeholder must always be %s in query.
    Warning: NEVER use Python string concatenation (+) or string parameters
    interpolation (%) to pass variables to a SQL query string.
    e.g.
      SELECT vs_url FROM vs_table WHERE vs_name = 'default_ge';
      query = "SELECT vs_url FROM vs_table WHERE vs_name = %s"
      parameters = ["default_ge"]

    Args:
      query: SQL SELECT statement.
      params: sequence of parameters to populate into placeholders.
    Returns:
      Results as list of tuples (rows of fields).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    con = None
    cursor = None

    query_results = []
    query_status = False

    self.logger.debug("Querying the database %s, at port %s, as user %s on"
                      "hostname %s" % (self._database, self._port, self._user,
                                       self._hostname))
    try:
      con = self._pool.getconn()
      if con:
        cursor = con.cursor()
        cursor.execute(query, params)

        for row in cursor:
          if len(row) == 1:
            query_results.append(row[0])
          else:
            query_results.append(row)
            query_status = True
    except psycopg2.pool.PoolError as e:
      self.logger.error("Exception while querying the database %s, %s",
                        self._database, e)
      raise exceptions.PoolConnectionException(
          "Pool Error - Unable to get a connection from the pool.")
    except psycopg2.Error as e:
      self.logger.error("Exception while querying the database %s, %s",
                        self._database, e)
    finally:
      if con:
        self._pool.putconn(con)

    return query_status, query_results

  def RunExampleSearch(self, search_query, response_type):
    """Performs a query search on the 'san_francisco_neighborhoods' table.

    Args:
      search_query: the query to be searched, in smallcase.
      response_type: Response type can be KML or JSONP, depending on the client
    Returns:
      tuple containing
      total_example_results: Total number of rows returned from
       querying the database.
      example_results: Query results as a list
    """
    example_results = []

    params = ["%" + entry + "%" for entry in search_query.split(",")]

    accum_func = self.utils.GetAccumFunc(response_type)

    example_query = self._example_query_template.substitute(FUNC=accum_func)
    query_status, query_results = self.RunPGSQLQuery(example_query, params)

    total_example_results = len(query_results)

    if query_status:
      for entry in xrange(total_example_results):
        results = {}

        name = query_results[entry][4]
        snippet = query_results[entry][3]
        styleurl = "#placemark_label"
        description = ("The total area in decimal degrees of " +
                       query_results[entry][4] + " is: " +
                       str(query_results[entry][1]) + "<![CDATA[<br/>]]>")
        description += ("The total perimeter in decimal degrees of " +
                        query_results[entry][4] + " is: " +
                        str(query_results[entry][2]))
        geom = str(query_results[entry][0])

        results["name"] = name
        results["snippet"] = snippet
        results["styleurl"] = styleurl
        results["description"] = description
        results["geom"] = geom
        results["geom_type"] = str(query_results[entry][5])

        example_results.append(results)

    return total_example_results, example_results

  def ConstructKMLResponse(self, search_results, original_query):
    """Prepares KML response.

    KML response has the below format:
      <kml>
       <Folder>
       <name/>
       <StyleURL>
             ---
       </StyleURL>
       <Point>
              <coordinates/>
       </Point>
       </Folder>
      </kml>

    Args:
     search_results: Query results from the searchexample database
     original_query: Search query as entered by the user
    Returns:
     kml_response: KML formatted response
    """
    search_placemarks = ""
    kml_response = ""
    lookat_info = ""
    set_first_element_lookat = True

    # folder name should include the query parameter(q) if 'displayKeys'
    # is present in the URL otherwise not.
    if self.display_keys_string:
      folder_name = ("Grouped results:<![CDATA[<br/>]]>%s (%s)"
                     % (original_query, str(len(search_results))))
    else:
      folder_name = ("Grouped results:<![CDATA[<br/>]]> (%s)"
                     % (str(len(search_results))))

    fly_to_first_element = str(self.fly_to_first_element).lower() == "true"

    for result in search_results:
      geom = ""
      placemark = ""

      geom = self._geom % (
          result["name"],
          result["styleurl"],
          result["snippet"],
          result["description"],
          result["geom"])

      # Add <LookAt> for POINT geometric types only.
      # TODO: Check if <LookAt> can be added for
      # LINESTRING and POLYGON types.
      if result["geom_type"] != "POINT":
        set_first_element_lookat = False

      if fly_to_first_element and set_first_element_lookat:
        lookat_info = self.utils.GetLookAtInfo(result["geom"])
        set_first_element_lookat = False

      placemark = self._placemark_template.substitute(geom=geom)
      search_placemarks += placemark

    kml_response = self._kml_template.substitute(
        foldername=folder_name,
        style=self._style,
        lookat=lookat_info,
        placemark=search_placemarks)

    self.logger.info("KML response successfully formatted")

    return kml_response

  def ConstructJSONPResponse(self, search_results, original_query):
    """Prepares JSONP response.

      {
               "Folder": {
                 "name": "Latitude X Longitude Y",
                 "Placemark": {
                    "Point": {
                      "coordinates": "X,Y" } }
                 }
       }
    Args:
     search_results: Query results from the searchexample table
     original_query: Search query as entered by the user
    Returns:
     jsonp_response: JSONP formatted response
    """
    search_placemarks = ""
    search_geoms = ""
    geoms = ""
    json_response = ""
    jsonp_response = ""

    folder_name = ("Grouped results:<![CDATA[<br/>]]>%s (%s)"
                   % (original_query, str(len(search_results))))

    for count, result in enumerate(search_results):
      geom = ""
      geom = self._json_geom % (
          result["name"],
          result["snippet"],
          result["description"],
          result["geom"][1:-1])

      if count < (len(search_results) -1):
        geom += ","
      geoms += geom

    if len(search_results) == 1:
      search_geoms = geoms
    else:
      search_geoms = "["+ geoms +"]"

    search_placemarks = self._json_placemark_template.substitute(
        geom=search_geoms)

    json_response = self._json_template.substitute(
        foldername=folder_name, json_placemark=search_placemarks)

    # Escape single quotes from json_response.
    json_response = json_response.replace("'", "\\'")

    jsonp_response = self._jsonp_call % (self.f_callback, json_response)

    self.logger.info("JSONP response successfully formatted")

    return jsonp_response

  def HandleSearchRequest(self, environ):
    """Fetches the search tokens from form and performs the example search.

    Args:
     environ: A list of environment variables as supplied by the
      WSGI interface to the example search application interface.
    Returns:
     search_results: A KML/JSONP formatted string which contains search results.
    """
    search_results = ""
    search_status = False

    # Fetch all the attributes provided by the user.
    parameters = self.utils.GetParameters(environ)
    response_type = self.utils.GetResponseType(environ)

    # Retrieve the function call back name for JSONP response.
    self.f_callback = self.utils.GetCallback(parameters)

    original_query = self.utils.GetValue(parameters, "q")

    # Fetch additional query parameters 'flyToFirstElement' and
    # 'displayKeys' from URL.
    self.fly_to_first_element = self.utils.GetValue(
        parameters, "flyToFirstElement")
    self.display_keys_string = self.utils.GetValue(
        parameters, "displayKeys")

    if not original_query:
      # Extract 'neighborhood' parameter from URL
      try:
        form = cgi.FieldStorage(fp=environ["wsgi.input"], environ=environ)
        original_query = form.getvalue("neighborhood")
      except AttributeError as e:
        self.logger.debug("Error in neighborhood query %s" % e)

    if original_query:
      (search_status, search_results) = self.DoSearch(
          original_query, response_type)
    else:
      self.logger.debug("Empty or incorrect search query received")

    if not search_status:
      folder_name = "No results were returned."
      search_results = self.utils.NoSearchResults(
          folder_name, self._style, response_type, self.f_callback)

    return (search_results, response_type)

  def DoSearch(self, original_query, response_type):
    """Performs the example search and returns the results.

    Args:
     original_query: A string containing the search query as
      entered by the user.
     response_type: Response type can be KML or JSONP, depending on the client.
    Returns:
     tuple containing
     search_status: Whether search could be performed.
     search_results: A KML/JSONP formatted string which contains search results.
    """
    search_status = False

    search_results = ""
    query_results = ""

    total_results = 0

    search_query = original_query.strip().lower()

    if len(search_query.split(",")) > 2:
      self.logger.warning("Extra search parameters ignored:%s"
                          % (",".join(search_query.split(",")[2:])))
      search_query = ",".join(search_query.split(",")[:2])
      original_query = ",".join(original_query.split(",")[:2])

    total_results, query_results = self.RunExampleSearch(
        search_query, response_type)

    self.logger.info("example search returned %s results"
                     % total_results)

    if total_results > 0:
      if response_type == "KML":
        search_results = self.ConstructKMLResponse(
            query_results, original_query)
        search_status = True
      elif response_type == "JSONP":
        search_results = self.ConstructJSONPResponse(
            query_results, original_query)
        search_status = True
      else:
        # This condition may not occur,
        # as response_type is either KML or JSONP
        self.logger.debug("Invalid response type %s" % response_type)

    return search_status, search_results

  def __del__(self):
    """Closes the connection pool created in __init__.
    """
    self._pool.closeall()


def main():
  expobj = ExampleSearch()
  expobj.DoSearch("pacific heights", "KML")

if __name__ == "__main__":
  main()
