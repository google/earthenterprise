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

"""Module for implementing the POI search."""

import cgi
import logging
import os
import re
from string import Template
import urlparse

import psycopg2
from psycopg2.pool import ThreadedConnectionPool

from search.common import exceptions
from search.common import geconstants
from search.common import utils

from search.plugin import federated_search_handler


class POISearch(object):
  """Class for performing the POI search.

  POI search queries the "gesearch" database for the sql queries
  which were created when the user configures the POI search tab
  and push/publishes from Fusion. These sql queries are executed
  on the "gepoi" database to retrieve the search results.

  Input parameters depend on what search fields have been chosen
  while configuring the POI seach using the Search Tab Manager in Fusion.

  For example, if latitude and longitude have been chosen as part of POI
  search fields, then

  Valid Inputs are :
  q=<latitude_value or longitude_value>&DbId=<db_id>
  or
  searchTerm=<latitude_value or longitude_value>&DbId=<db_id>
  """

  def __init__(self):
    """Inits POISearch.

    Initializes the logger "ge_search".
    Initializes templates for kml, placemark templates for the KML output.
    Initializes parameters for establishing a connection to the database.
    """

    self.utils = utils.SearchUtils()
    constants = geconstants.Constants()

    configs = self.utils.GetConfigs(
        os.path.join(geconstants.SEARCH_CONFIGS_DIR, "PoiSearch.conf"))

    style_template = self.utils.style_template
    self._jsonp_call = self.utils.jsonp_functioncall

    self._geom = """
            <name>%s</name>,
            <styleUrl>%s</styleUrl>
            <description>%s</description>,
            %s\
    """
    self._json_geom = """
          {
            "name" : "%s",
            "description" : "%s",
            %s
          }
    """

    self._placemark_template = self.utils.placemark_template
    self._kml_template = self.utils.kml_template

    self._json_template = self.utils.json_template
    self._json_placemark_template = self.utils.json_placemark_template

    self._host_db_name_by_target_query = constants.host_db_name_by_target_query
    self._poi_info_by_host_db_name_query = (
        constants.poi_info_by_host_db_name_query)

    self.logger = logging.getLogger("ge_search")

    poisearch_database = configs.get("searchdatabasename")
    if not poisearch_database:
      poisearch_database = constants.defaults.get("poisearch.database")

    gestream_database = configs.get("streamdatabasename")
    if not gestream_database:
      gestream_database = constants.defaults.get("gestream.database")

    poiquery_database = configs.get("poidatabasename")
    if not poiquery_database:
      poiquery_database = constants.defaults.get("poiquery.database")

    self._search_pool = ThreadedConnectionPool(
        int(configs.get("minimumconnectionpoolsize")),
        int(configs.get("maximumconnectionpoolsize")),
        database=poisearch_database,
        user=configs.get("user"),
        host=configs.get("host"),
        port=int(configs.get("port")))

    self._poi_pool = ThreadedConnectionPool(
        int(configs.get("minimumconnectionpoolsize")),
        int(configs.get("maximumconnectionpoolsize")),
        database=poiquery_database,
        user=configs.get("user"),
        host=configs.get("host"),
        port=int(configs.get("port")))

    self._stream_pool = ThreadedConnectionPool(
        int(configs.get("minimumconnectionpoolsize")),
        int(configs.get("maximumconnectionpoolsize")),
        database=gestream_database,
        user=configs.get("user"),
        host=configs.get("host"),
        port=int(configs.get("port")))

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

    # Parameters for calculating the bounding box.
    self.latitude_center = constants.latitude_center
    self.longitude_center = constants.longitude_center
    self.latitude_span = constants.latitude_span
    self.longitude_span = constants.longitude_span
    self.srid = constants.srid
    self.usebbox = configs.get("usebbox").lower() == "true"
    self.expandbbox = configs.get("expandbbox").lower() == "true"

    # Calculate default bounding box with latitude span = 180(degrees) and
    # longitude span = 360(degrees).

    self.world_bounds = self.__CreateBboxFromParameters(
        self.latitude_center, self.longitude_center,
        self.latitude_span, self.longitude_span)

    # Create federated search handler object for
    # performing super federated search.
    try:
      self._fed_search = federated_search_handler.FederatedSearch()
    except Exception as e:
      self.logger.warning("Federated search is not available due to an "
                          "unexpected error, %s.", e)
      self._fed_search = None

  def __RunPGSQLQuery(self, pool, query, params):
    """Submits the query to the database and returns tuples.

    Note: variables placeholder must always be %s in query.
    Warning: NEVER use Python string concatenation (+) or string parameters
    interpolation (%) to pass variables to a SQL query string.
    e.g.
      SELECT vs_url FROM vs_table WHERE vs_name = 'default_ge';
      query = "SELECT vs_url FROM vs_table WHERE vs_name = %s"
      parameters = ["default_ge"]

    Args:
      pool: Pool of database connection's on which the query should be executed.
      query: SQL SELECT statement.
      params: sequence of parameters to populate into placeholders.
    Returns:
      Results as list of tuples (rows of fields).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    cursor = None

    query_results = []
    query_status = False

    try:
      con = pool.getconn()
      if con:
        cursor = con.cursor()

        dsn_info = {}
        for entry in con.dsn.split(" "):
          dsn_entry = entry.split("=")
          dsn_info[dsn_entry[0]] = dsn_entry[1]

        self.logger.debug("Querying the database %s, at port %s, as user %s on "
                          "hostname %s.", dsn_info["dbname"], dsn_info["port"],
                          dsn_info["user"], dsn_info["host"])
        self.logger.debug("Query: %s", query)
        self.logger.debug("Params: %s", params)
        cursor.execute(query, params)

        for row in cursor:
          if len(row) == 1:
            query_results.append(row[0])
          else:
            query_results.append(row)
          query_status = True
    except psycopg2.pool.PoolError as e:
      self.logger.error("Exception while querying the database, %s", e)
      raise exceptions.PoolConnectionException(
          "Pool Error - Unable to get a connection from the pool.")
    except psycopg2.Error as e:
      self.logger.error("Exception while querying the database, %s", e)
    finally:
      if con:
        pool.putconn(con)

    return query_status, query_results

  def __QueryPOIInfo(self, target_path):
    """Gets POI info data for specified target.

      Queries gestream database to get database info (host_name, db_name)
    by target path, then queries gesearch database to get poi info data
    from poi_table by database info.

    Args:
     target_path: Published target path.
    Returns:
     query_status: True if POI info data has been fetched.
     poi_info_data: poi info data as a list of tuples
       (query_str, num_fields) or empty list.
    Raises:
      psycopg2.pool.PoolError in case of error while getting a
       connection from the pool.
    """
    query_status = False
    poi_info_data = []

    # Get host_name and db_name from gestream database for a target path.
    query_status, query_data = self.__RunPGSQLQuery(
        self._stream_pool,
        self._host_db_name_by_target_query,
        (target_path,))

    if not query_status:
      self.logger.debug("Target path %s not published yet", target_path)
    else:
      # query_data is of type [(host_name, db_name)].
      # Sample result is [('my_host',
      # '/gevol/assets/Databases/my_db.kdatabase/gedb.kda/ver002/gedb')]

      # Fetch (query_str, num_fields) for POISearch from gesearch database
      # with host_name and db_name as inputs.
      query_status, poi_info_data = self.__RunPGSQLQuery(
          self._search_pool,
          self._poi_info_by_host_db_name_query,
          query_data[0])

    return query_status, poi_info_data

  def __ConstructResponse(self, poi_results, response_type, original_query):
    """Prepares response based on response type.

    Args:
     poi_results: Query results from the gepoi database.
     response_type: KML or JSONP.
     original_query: Search query as entered by the user.
    Returns:
     format_status: If response has been properly formatted(True/False).
     format_response: Response in KML or JSON.
    """
    format_status = False
    format_response = ""

    # Remove quotes so they don't interfere with kml or json.
    safe_query = original_query.replace('"', "").replace("'", "")
    if response_type == "KML":
      format_response = self.__ConstructKMLResponse(poi_results, safe_query)
      format_status = True
    elif response_type == "JSONP":
      format_response = self.__ConstructJSONPResponse(poi_results, safe_query)
      format_status = True
    else:
      # This condition may not occur,
      # as response_type is either KML or JSONP.
      self.logger.error("Invalid response type %s", response_type)

    return format_status, format_response

  def __ConstructKMLResponse(self, search_results, original_query):
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
     search_results: Query results from the gepoi database.
     original_query: Search query as entered by the user.
    Returns:
     kml_response: KML formatted response.
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

      styleurl = "#placemark_label"
      description_template = Template("${NAME} = ${VALUE}\n")
      name, description, geom_data = self.__GetPOIAttributes(
          original_query, result, description_template)
      geom = self._geom % (cgi.escape(name), styleurl, cgi.escape(description),
                           geom_data)

      if fly_to_first_element and set_first_element_lookat:
        lookat_info = self.utils.GetLookAtInfo(geom_data)
        set_first_element_lookat = False

      placemark = self._placemark_template.substitute(geom=geom)
      search_placemarks += placemark

    kml_response = self._kml_template.substitute(
        foldername=folder_name,
        style=self._style,
        lookat=lookat_info,
        placemark=search_placemarks)

    self.logger.debug("KML response successfully formatted")

    return kml_response

  def __ConstructJSONPResponse(self, search_results, original_query):
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
     search_results: Query results from the gepoi table.
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

    # If end user submits multiple queries in one POI Search, like x,y
    # then take x only for POI Search.
    search_tokens = self.utils.SearchTokensFromString(original_query)

    for count, result in enumerate(search_results):
      geom = ""

      description_template = Template("${NAME}: ${VALUE}<br>")

      name, description, geom_data = self.__GetPOIAttributes(
          search_tokens[0], result, description_template)

      # Remove the outer braces for the json geometry.
      geom_data = geom_data.strip().lstrip("{").rstrip("}")
      geom = self._json_geom % (name, description, geom_data)

      geoms += geom
      if count < (len(search_results) - 1):
        geoms += ","

    if len(search_results) == 1:
      search_geoms = geoms
    else:
      search_geoms = "[" + geoms + "]"

    search_placemarks = self._json_placemark_template.substitute(
        geom=search_geoms)

    json_response = self._json_template.substitute(
        foldername=folder_name,
        json_placemark=search_placemarks)

    # Escape single quotes from json_response.
    json_response = json_response.replace("'", "\\'")

    jsonp_response = self._jsonp_call % (self.f_callback, json_response)

    self.logger.debug("JSONP response successfully formatted")

    return jsonp_response

  def __GetPOIAttributes(self, original_query, search_result, desc_template):
    """Fetch POI attributes like name, description and geometry.

    Args:
     original_query: Search query as entered by the user.
     search_result: Query results from the gepoi table.
     desc_template: Description template based on the response type.
    Returns:
     Name, description and geometry.
    """
    name = ""
    description = ""

    # A sample map_entry would be as below.
    # {
    #          'field_value': 'US Route 395',
    #          'field_name': 'name',
    #          'is_search_display': True,
    #          'is_searchable': True,
    #          'is_displayable': True
    # }

    for map_entry in search_result:
      if map_entry["field_name"] != "geom":
        if map_entry["is_search_display"]:
          # Find the matched string by checking if the search query
          # is a sub-string of the POI field values.

          # This condition is for POI fields which are both
          # searchable and displayable.
          if original_query.lower() in map_entry["field_value"].lower():
            name = map_entry["field_value"]
        else:
          # This condition is for POI fields which are
          # searchable but not displayable.

          # Assign "name" attribute to the first POI field value.
          if not name:
            name = search_result[1]["field_value"]

        # Assign all the displayable POI field values
        # to "description" attribute.
        if map_entry["is_displayable"]:
          description += desc_template.substitute(
              NAME=map_entry["field_name"], VALUE=map_entry["field_value"])
      else:
        geom_data = map_entry["field_value"]

    return name, description, geom_data

  def HandleSearchRequest(self, environ):
    """Fetches the search tokens from form and performs the POI search.

    Args:
     environ: A list of environment variables as supplied by the
      WSGI interface to the POI search application interface.
    Returns:
     search_results: A KML/JSONP formatted string which contains search results.
     response_type: KML or JSON, depending on the end client.
    """
    search_results = ""
    search_status = False
    is_super_federated = False

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

    # Process bounding box flag and parameters.
    # Get useBBox and expandBBox from query.
    # useBBox and expandBBox can be set in the additional config parameters
    # section while creating the search tabs in server admin UI.

    # Any value specified here should override the value
    # provided in the PoiSearch.conf config file.
    usebbox = self.utils.GetValue(parameters, "useBBox")
    if usebbox:
      self.usebbox = usebbox.lower() == "true"

    # Get expandBBox only when useBBox is True.
    if self.usebbox:
      expandbbox = self.utils.GetValue(parameters, "expandBBox")
      if expandbbox:
        self.expandbbox = expandbbox.lower() == "true"

    # Default bbox is world_bounds.
    bbox = self.world_bounds

    if self.usebbox:
      # Get ll and spn params from the query.
      lat_lng = self.utils.GetValue(parameters, "ll")
      spans = self.utils.GetValue(parameters, "spn")
      if lat_lng and spans:
        lat_lng = [float(coord) for coord in lat_lng.split(",")]
        spans = [float(span) for span in spans.split(",")]
        # Override default bounding box setting.
        bbox = self.__CreateBboxFromParameters(
            lat_lng[0], lat_lng[1], spans[0], spans[1])

    if not original_query:
      # If "q" not available, extract 'searchTerm' parameter value from URL.
      original_query = self.utils.GetValue(parameters, "searchTerm")

    # Extract target path from 'SCRIPT_URL'.
    # SAMPLE SCRIPT_URL IS '/sf2d/POISearch'.

    parse_res = urlparse.urlparse(environ["SCRIPT_URL"])
    match_tp = re.match(r"(.*)/POISearch", parse_res.path)
    if match_tp:
      target_path = match_tp.groups()[0]

    poi_info_status, poi_info_data = self.__QueryPOIInfo(target_path)

    if original_query and poi_info_status:
      end_search = False
      while True:
        (search_status, search_results) = self.DoSearch(
            original_query, poi_info_data, response_type, bbox)

        # If there are no search results and expandbbox is true,
        # expand the search region by providing a new bounding box
        # whose latitude and longitude values are double that of the
        # original. Continue this process until atleast a single search
        # result is found.
        if (not search_status and self.usebbox and
            self.expandbbox and not end_search):
          # provide new bbox with latitude and longitude span values multiplied
          # by a factor of 2.
          spans = [span*2 for span in spans]
          if self.__IsBBoxValid(lat_lng[0], lat_lng[1], spans[0], spans[1]):
            bbox = self.__CreateBboxFromParameters(
                lat_lng[0], lat_lng[1], spans[0], spans[1])
          else:
            # Try search with world bounds.
            bbox = self.world_bounds
            end_search = True
        else:
          break
    else:
      self.logger.error("Empty or incorrect search query received.")

    if search_status:
      (search_status, search_results) = self.__ConstructResponse(
          search_results, response_type, original_query)
    else:
      # If no results from POI search, then perform Federated search.
      if original_query and self._fed_search:
        # Get "PoiFederated" parameter value.
        # "PoiFederated" can take values 0 or 1.
        super_federated_value = self.utils.GetValue(
            parameters, "PoiFederated")
        if super_federated_value:
          # For any value not greater than 0,is_super_federated is 'False'
          is_super_federated = int(super_federated_value) > 0

        # Perform Federated search if the POI search results in
        # zero(0) results and 'is_super_federated' value is 'True'.
        if is_super_federated:
          self._fed_search.f_callback = self.f_callback
          self._fed_search.fly_to_first_element = self.fly_to_first_element
          self._fed_search.display_keys_string = self.display_keys_string
          (search_status, search_results) = self._fed_search.DoSearch(
              original_query, response_type)

    # if no results from POI search or Federated search.
    if not search_status:
      folder_name = "No results were returned."
      search_results = self.utils.NoSearchResults(
          folder_name, self._style, response_type, self.f_callback)

    return (search_results, response_type)

  def DoSearch(self, search_expression, poi_info_data, response_type, bbox):
    """Performs the poi search and returns the results.

    Args:
     search_expression: Search query as entered by the user.
     poi_info_data: POI info as a list of tuples (query_str, num_fields).
     response_type: Response type can be KML or JSONP, depending on the client.
     bbox: The bounding box string.
    Returns:
     sql_query_status: Whether data could be queried from "gepoi" database.
     poi_results: Query results as a list.
    """

    sql_query_status = False
    poi_data = []

    normalised_search_expression = search_expression.lower()
    for sql_query in poi_info_data:
      cur_status, cur_data = self.__ParseSQLResults(
          normalised_search_expression, sql_query, response_type, bbox)
      if cur_status:
        sql_query_status = cur_status
        poi_data += cur_data

    self.logger.debug("poi search returned %s results", len(poi_data))
    self.logger.debug("results: %s", poi_data)

    return sql_query_status, poi_data

  def __ParseSQLResults(self, search_expression, poi_info, response_type, bbox):
    """Performs the poi search and returns the results.

    Args:
     search_expression: Normalized search expression.
     poi_info: Tuple containing sql_query, number of columns, style id
      and poi id as extracted from "poi_table" table of "gesearch" database.
     response_type: Either KML/JSONP, depending on the client.
     bbox: The bounding box string.
    Returns:
     poi_query_status: Whether data could be queried from "gepoi" database.
     poi_query_results: Query results as a list.
    Raises:
      psycopg2.pool.PoolError in case of error while getting a
          connection from the pool.
    """
    poi_query_status = False
    poi_query_results = []
    query_results = []
    poi_query = ""

    sql_query = poi_info[0]
    num_of_input_fields = poi_info[1]

    try:
      search_tokens = self.utils.SearchTokensFromString(search_expression)
    except Exception as e:
      raise exceptions.BadQueryException(
          "Couldn't parse search term. Error: %s" % e)

    self.logger.debug("Parsed search tokens: %s", search_tokens)
    params = ["%" + entry + "%" for entry in search_tokens]

    num_params = len(params)
    if num_params == 0:
      raise exceptions.BadQueryException("No search term.")

    if num_params > num_of_input_fields:
      params = params[:num_of_input_fields]
    elif num_params < num_of_input_fields:
      if num_params == 1:
        params.extend([params[0]] * (num_of_input_fields - num_params))
      else:
        params.extend(["^--IGNORE--"] * (num_of_input_fields - num_params))

    accum_func = self.utils.GetAccumFunc(response_type)

    # sql queries retrieved from "gesearch" database has "?" for
    # input arguments, but postgresql supports "%s".
    # Hence, replacing "?" with "%s".

    sql_stmt = sql_query.replace("?", "%s")

    # Extract the displayable and searchable POI fields from the POI
    # queries retrieved from "poi_table".
    matched = re.match(r"(\w*)\s*(Encode.*geom)(.*)(FROM.*)(WHERE )(\(.*\))",
                       sql_stmt)

    if matched:
      (sub_query1, unused_sub_query2,
       sub_query3, sub_query4, sub_query5, sub_query6) = matched.groups()
      # sub_query2 need to be replaced with a new query
      # as per the response_type.
      # PYLINT throws warning that sub_query2 has not been used, it's
      # not required and can be ignored.

      geom_stmt = "%s(the_geom) AS the_geom" % (accum_func)
      poi_query = "%s %s%s%s%s%s" % (sub_query1, geom_stmt, sub_query3,
                                     sub_query4, sub_query5, sub_query6)

      poi_query += " AND ( the_geom && %s )" % bbox

      # Displayable POI fields appear between SELECT and FROM
      # clause of the POI queries, as in below example.
      # SELECT ST_AsKML(the_geom) AS the_geom, "rpoly_", "fnode_"
      # FROM gepoi_7 WHERE ( lower("rpoly_") LIKE %s OR lower("name") LIKE %s ).

      # "rpoly_" and "fnode_" are display fields in the above example.
      display_fields = [field.replace("\"", "").strip()
                        for field in filter(len, sub_query3.split(","))]
      # Insert geom at the 0th index for easy retrieval.
      display_fields.insert(0, "geom")

      # Searchable POI fields appear after the WHERE
      # clause of the POI queries, as in below example.
      # SELECT ST_AsKML(the_geom) AS the_geom, "rpoly_", "fnode_"
      # FROM gepoi_7 WHERE ( lower("rpoly_") LIKE %s OR lower("name") LIKE %s ).

      # "rpoly_" and "name" are searchable fields in the above example.
      searchable_fields = [
          entry.strip().strip("OR").strip().strip("lower").strip("\\(\\)\"")
          for entry in filter(len, sub_query6.strip("\\(\\) ").split("LIKE %s"))
          ]

      # Insert geom at the 0th index for easy retrieval.
      searchable_fields.insert(0, "geom")

    if poi_query:
      poi_query_status, query_results = self.__RunPGSQLQuery(
          self._poi_pool, poi_query, params)

      # Create a map which will have the POI fields values
      # retrieved from the gepoi_X tables and
      # other information like displayable or searchable or both
      # based on the display and search labels retrieved above.

      # Some sample maps are as below.
      # 1) {
      #    'field_value': 'State Route 55',
      #    'field_name': 'name',
      #  'is_search_display': True,
      #   'is_searchable': True,
      #   'is_displayable': True
      #}

      # 2) {'field_value': '0',
      #    'field_name': 'rpoly_',
      #    'is_search_display': True,
      #    'is_searchable': True,
      #    'is_displayable': True}.

      # 3) {'field_value': '22395',
      #     'field_name': 'fnode_',
      #     'is_search_display': True,
      #     'is_searchable': True,
      #     'is_displayable': True}.

      # These maps would be used when creating the KML and JSONP responses
      # The different flags (is_displayable,is_searchable etc) allow for
      # easier retrieval of data based on our requirements.

      for entry in query_results:
        field_name_value = []

        for field_name, field_value in zip(display_fields, entry):
          temp = {}
          temp["field_name"] = field_name
          temp["field_value"] = field_value
          temp["is_searchable"] = (field_name in searchable_fields)
          # "is_displayable" is always True as we are iterating over
          # the display(only) POI fields.
          temp["is_displayable"] = True
          temp["is_search_display"] = (
              temp["is_displayable"] and temp["is_searchable"])
          field_name_value.append(temp)

        for field_name in searchable_fields:
          temp = {}
          if field_name not in display_fields:
            temp["field_name"] = field_name
            temp["field_value"] = ""
            # "is_searchable" is always True as we are iterating over
            # the search(only) POI fields.
            temp["is_searchable"] = True
            temp["is_displayable"] = (field_name in display_fields)
            temp["is_search_display"] = (
                temp["is_displayable"] and temp["is_searchable"])
            field_name_value.append(temp)

        poi_query_results.append(field_name_value)

    return poi_query_status, poi_query_results

  def __CreateBboxFromParameters(self, latcenter, loncenter, latspan, lonspan):
    """Create a bounding box string for bounding box queries.

    Args:
      latcenter: latitude centre in degrees.
      loncenter: longitude centre in degrees.
      latspan: full latitude span in degrees.
      lonspan: full longitude span in degrees.
    Returns:
      The bounding box string.
    """
    (xmin, xmax, ymin, ymax) = self.__GetBBoxBounds(
        latcenter, loncenter, latspan, lonspan)
    bbox = "ST_SetSRID('BOX3D(%s %s,%s %s)'::box3d,%s)" %(
        xmin, ymin, xmax, ymax, self.srid)

    return bbox

  def __GetBBoxBounds(self, latcenter, loncenter, latspan, lonspan):
    """Get bounding box coordinates.

    Args:
      latcenter: latitude centre in degrees.
      loncenter: longitude centre in degrees.
      latspan: full latitude span in degrees.
      lonspan: full longitude span in degrees.
    Returns:
      The bounding box coordinates.
    """
    ymin = "%.1f" %(latcenter - (latspan / 2))
    ymax = "%.1f" %(latcenter + (latspan / 2))
    xmin = "%.1f" %(loncenter - (lonspan / 2))
    xmax = "%.1f" %(loncenter + (lonspan / 2))

    return (float(xmin), float(xmax), float(ymin), float(ymax))

  def __IsBBoxValid(self, latcenter, loncenter, latspan, lonspan):
    """Check if the bounding box is valid.

    Args:
      latcenter: latitude centre in degrees.
      loncenter: longitude centre in degrees.
      latspan: full latitude span in degrees.
      lonspan: full longitude span in degrees.
    Returns:
      Validity of the bounding box.
    """
    is_bbox_valid = False

    (xmin, xmax, ymin, ymax) = self.__GetBBoxBounds(
        latcenter, loncenter, latspan, lonspan)
    if xmin >= -180.0 and xmax <= 180.0 and ymin >= -90.0 and ymax <= 90.0:
      is_bbox_valid = True
    return is_bbox_valid

  def __del__(self):
    """Closes the connection pool created in __init__.
    """
    if self._poi_pool:
      self._poi_pool.closeall()
    if self._search_pool:
      self._search_pool.closeall()
    if self._stream_pool:
      self._stream_pool.closeall()


def main():
  poiobj = POISearch()
  bbox = "ST_SetSRID('BOX3D(-180.0 -90.0,180.0 90.0)'::box3d,4326)"
  poiobj.DoSearch("US Route 101", 7, "KML", bbox)

if __name__ == "__main__":
  main()
