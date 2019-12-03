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

"""Module for implementing the GEPlaces search."""

import os
from string import Template
import psycopg2
from psycopg2.pool import ThreadedConnectionPool
from search.common import exceptions
from search.common import geconstants
from search.common import utils


class PlacesSearch(object):
  """Class for performing the GEPlaces search.

  Search can be performed either as a single scope search where the search
  string is either a city or country or either as a double scope search where
  the search is performed for a city in a state/country combination.

  Valid Inputs are :
  q = santa clara
  q = Atlanta, Georgia
  """

  def __init__(self):
    """Inits GEPlaces.

    Initializes the logger "ge_search".
    Initializes templates for kml,placemark templates for the KML output.
    Initializes parameters for establishing a connection to the database.
    """

    self.utils = utils.SearchUtils()
    constants = geconstants.Constants()
    configs = self.utils.GetConfigs(
        os.path.join(geconstants.SEARCH_CONFIGS_DIR, "GEPlacesSearch.conf"))

    self._jsonp_call = self.utils.jsonp_functioncall
    self._geom = """
            <name>%s</name>
            <styleUrl>%s</styleUrl>
            <description>%s</description>
            %s\
    """
    self._json_geom = """
         {
          "name": "%s",
          "description": "%s",
           %s
         }\
    """

    self._placemark_template = self.utils.placemark_template
    self._kml_template = self.utils.kml_template
    style_template = self.utils.style_template

    self._json_template = self.utils.json_template
    self._json_placemark_template = self.utils.json_placemark_template

    self._city_query_template = (
        Template(constants.city_query))
    self._country_query_template = (
        Template(constants.country_query))
    self._city_and_country_name_query_template = (
        Template(constants.city_and_country_name_query))
    self._city_and_country_code_query_template = (
        Template(constants.city_and_country_code_query))
    self._city_and_subnational_name_query_template = (
        Template(constants.city_and_subnational_name_query))
    self._city_and_subnational_code_query_template = (
        Template(constants.city_and_subnational_code_query))

    self.logger = self.utils.logger

    self._user = configs.get("user")
    self._hostname = configs.get("host")
    self._port = configs.get("port")

    self._database = configs.get("databasename")
    if not self._database:
      self._database = constants.defaults.get("places.database")

    self._pool = ThreadedConnectionPool(
        int(configs.get("minimumconnectionpoolsize")),
        int(configs.get("maximumconnectionpoolsize")),
        database=self._database,
        user=self._user,
        host=self._hostname,
        port=int(self._port))

    self.style = style_template.substitute(
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
      Query execution status and list of tuples of query results.
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    con = None
    cursor = None

    query_results = []
    query_status = False

    self.logger.debug("Querying the database %s,at port %s,as user %s on "
                      "hostname %s", self._database, self._port, self._user,
                      self._hostname)
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

  def RunCityGeocoder(self, search_query, response_type):
    """Performs a query search on the 'cities' table.

    Args:
      search_query: the query to be searched, in small case.
      response_type: Response type can be KML or JSONP, depending on the client.
    Returns:
      tuple containing
      total_city_results: Total number of rows returned from
       querying the database.
      city_results: Query results as a list.
    """
    city_results_list = []

    search_city_view = self.utils.GetCityView(search_query)

    params = search_query.split(",")

    accum_func = self.utils.GetAccumFunc(response_type)

    city_query = self._city_query_template.substitute(
        FUNC=accum_func, CITY_VIEW=search_city_view)

    query_status, query_results = self.RunPGSQLQuery(city_query, params)
    total_city_results = len(query_results)

    if query_status:
      for entry in query_results:
        city_results = {}

        if entry[3] == 0:
          city_population = "unknown"
        else:
          city_population = str(entry[3])

        name = "%s, %s" % (entry[1], entry[4])
        styleurl = "#placemark_label"
        country_name = "Country: %s" % (entry[2])
        population = "Population: %s" % (city_population)
        description = "%s<![CDATA[<br/>]]>%s" % (country_name, population)
        geom = str(entry[0])

        city_results["name"] = name
        city_results["styleurl"] = styleurl
        city_results["description"] = description
        city_results["geom"] = geom
        city_results["geom_type"] = entry[5]

        city_results_list.append(city_results)

    return total_city_results, city_results_list

  def RunCountryGeocoder(self, search_query, response_type):
    """Performs a query search on the 'countries' table.

    Args:
      search_query: the query to be searched, in smallcase.
      response_type: Response type can be KML or JSONP, depending on the client.
    Returns:
      tuple containing
      total_country_results: Total number of rows returned from
       querying the database.
      country_results_list: Query results as a list.
    """
    country_results_list = []

    params = search_query.split(",")
    accum_func = self.utils.GetAccumFunc(response_type)

    country_query = self._country_query_template.substitute(FUNC=accum_func)
    query_status, query_results = self.RunPGSQLQuery(country_query, params)

    total_country_results = len(query_results)

    if query_status:
      for entry in query_results:
        country_results = {}

        if entry[5] == 0:
          country_population = "unknown"
        else:
          country_population = str(entry[5])

        name = entry[0]
        styleurl = "#placemark_label"
        capital = "Capital: %s" % (entry[3])
        population = "Population: %s" % (country_population)
        description = "%s<![CDATA[<br/>]]>%s" % (capital, population)
        geom = str(entry[2])

        country_results["name"] = name
        country_results["styleurl"] = styleurl
        country_results["description"] = description
        country_results["geom"] = geom
        country_results["geom_type"] = entry[8]

        country_results_list.append(country_results)

    return total_country_results, country_results_list

  def SingleScopeSearch(self, search_query, response_type):
    """Performs a query search on the 'cities' and 'countries' tables.

    Input contains either city name or state name
    like "q=santa clara" or "q=California".

    Args:
      search_query: the query to be searched, in smallcase.
      response_type: Response type can be KML or JSONP, depending on the client.
    Returns:
      total_results: Total query results.
      single_scope_results: A list of dictionaries containing the 'Placemarks'.
    """
    total_results = 0
    single_scope_results = []

    country_count, country_results = self.RunCountryGeocoder(
        search_query, response_type)
    total_results += country_count

    city_count, city_results = self.RunCityGeocoder(
        search_query, response_type)
    total_results += city_count

    single_scope_results = country_results + city_results

    self.logger.info("places search returned %s results", total_results)

    return total_results, single_scope_results

  def DoubleScopeSearch(self, search_query, response_type):
    """Performs a query search on the 'cities' and 'countries' tables.

    Input contains both city name and state name,
    like "q=santa clara,California".

    Args:
      search_query: the query which contains both city name and state
        name to be search in the geplaces database.
      response_type: Response type can be KML or JSONP, depending on the client.
    Returns:
      total_results: Total query results.
      double_scope_results: A list of dictionaries containing the 'Placemarks'.
    """
    total_results = 0
    double_scope_results = []

    search_city_view = self.utils.GetCityView(search_query)

    params = [entry.strip() for entry in search_query.split(",")]
    params = params[0:2]*4

    accum_func = self.utils.GetAccumFunc(response_type)

    city_and_country_name_query = (
        self._city_and_country_name_query_template.substitute(
            FUNC=accum_func, CITY_VIEW=search_city_view))
    city_and_country_code_query = (
        self._city_and_country_code_query_template.substitute(
            FUNC=accum_func, CITY_VIEW=search_city_view))
    city_and_subnational_name_query = (
        self._city_and_subnational_name_query_template.substitute(
            FUNC=accum_func, CITY_VIEW=search_city_view))
    city_and_subnational_code_query = (
        self._city_and_subnational_code_query_template.substitute(
            FUNC=accum_func, CITY_VIEW=search_city_view))

    query = ("%s UNION %s UNION %s UNION %s ORDER BY population DESC"
             % (city_and_country_name_query,
                city_and_country_code_query,
                city_and_subnational_name_query,
                city_and_subnational_code_query))

    query_status, query_results = self.RunPGSQLQuery(query, params)
    total_results += len(query_results)

    if query_status:
      for entry in query_results:
        temp_results = {}

        if entry[1] == 0:
          population = "unknown"
        else:
          population = str(entry[1])

        name = "%s, %s" % (entry[2], entry[4])
        country_name = "Country: %s" % (entry[3])
        population = "Population: %s" % (population)
        description = "%s<![CDATA[<br/>]]>%s" % (country_name, population)
        styleurl = "#placemark_label"
        geom = str(entry[0])
        temp_results["name"] = name
        temp_results["styleurl"] = styleurl
        temp_results["description"] = description
        temp_results["geom"] = geom
        temp_results["geom_type"] = entry[5]

        double_scope_results.append(temp_results)

    self.logger.info("places search returned %s results", total_results)

    return total_results, double_scope_results

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
     search_results: Query results from the geplaces database.
     original_query: Search query as entered by the user.
    Returns:
     kml_response:KML formatted response.
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
        style=self.style,
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
     search_results: Query results from the geplaces table.
     original_query: Search query as entered by the user.
    Returns:
     jsonp_response: JSONP formatted response.
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
          result["description"],
          # we skip the beginning and end curly braces '{'
          # that are seen in the response from querying the database.
          result["geom"][1:-1])

      if count < (len(search_results) - 1):
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
    """Fetches the search tokens from form and performs the places search.

    Args:
     environ: A list of environment variables as supplied by the
      WSGI interface to the places search application interface.
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

    if original_query:
      (search_status, search_results) = self.DoSearch(
          original_query, response_type)
    else:
      self.logger.debug("Empty search query received")

    if not search_status:
      folder_name = "No results were returned."
      search_results = self.utils.NoSearchResults(
          folder_name, self.style, response_type, self.f_callback)

    return (search_results, response_type)

  def DoSearch(self, original_query, response_type):
    """Performs the places search and returns the results.

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

    if search_query:
      if len(search_query.split(",")) > 1:
        if len(search_query.split(",")) > 2:
          self.logger.warning("Extra search parameters ignored:%s",
                              ",".join(search_query.split(",")[2:]))
          search_query = ",".join(search_query.split(",")[:2])
          original_query = ",".join(original_query.split(",")[:2])
        total_results, query_results = self.DoubleScopeSearch(
            search_query, response_type)
      else:
        total_results, query_results = self.SingleScopeSearch(
            search_query, response_type)

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
        self.logger.error("Invalid response type %s", response_type)

    return search_status, search_results

  def __del__(self):
    """Closes the connection pool created in __init__.
    """
    self._pool.closeall()


def main():
  gepobj = PlacesSearch()
  gepobj.DoSearch("santa clara", "KML")

if __name__ == "__main__":
  main()
