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


"""Constants Module for common constants used in the search functionality."""
from common import postgres_properties

SEARCH_CONFIGS_DIR = "/opt/google/search/conf"


class Constants(object):
  """Class contains Constants used in search implementations."""

  def __init__(self):
    """Inits constants used in search implementation."""

    self.defaults = {
        # database settings default values.
        "port": postgres_properties.PostgresProperties().GetPortNumber(),
        "host": postgres_properties.PostgresProperties().GetHost(),
        "user": "geuser",
        "pass": postgres_properties.PostgresProperties().GetPassword(),
        "minimumconnectionpoolsize": 1,
        "maximumconnectionpoolsize": 20,
        "places.database": "geplaces",
        "example.database": "searchexample",
        "poiquery.database": "gesearch",
        "poisearch.database": "gepoi",
        "gestream.database": "gestream",
        "useBBox": "false",
        "expandBBox": "false",
        # Style settings default values.
        "balloonstyle.bgcolor": "ffffffff",
        "balloonstyle.textcolor": "ff000000",
        "balloonstyle.text": "$[description]",
        "iconstyle.scale": 1,
        "iconstyle.href": "../shared_assets/images/location_pin.png",
        "linestyle.width": 5,
        "linestyle.color": "7fFFFF00",
        "polystyle.color": "7f66ffff",
        "polystyle.colormode": "normal",
        "polystyle.fill": "true",
        "polystyle.outline": "true"
    }
    # Parameters for bounding box.
    self.latitude_center = 0
    self.longitude_center = 0
    self.latitude_span = 180
    self.longitude_span = 360
    self.srid = 4326

    # TODO: Add indexes for the queries as below
    # self.city_query_indexes = {
    #    "the_geom": 0,
    #    "city": 1,
    #    "country_name": 2,
    #    "population": 3,
    #    "subnational_name": 4,
    #    "geom_type": 5
    # }

    # So results for city_query are accessed with something like
    # city = result[city_query_indexes["city"]]

    self.city_query = (
        "SELECT ${FUNC}(ST_Force3DZ(the_geom)) AS the_geom,city,country_name,"
        "population, subnational_name, GeometryType(the_geom) AS geom_type "
        "FROM ${CITY_VIEW} "
        "WHERE "
        "lower(city) = %s "
        "ORDER BY population DESC")

    self.country_query = (
        "SELECT country_name, country_code, "
        "${FUNC}(ST_Force3DZ(the_geom)) AS the_geom, capital, areainsqkm, "
        "population, continent_name, languages, "
        "GeometryType(the_geom) AS geom_type "
        "FROM countries "
        "WHERE "
        "lower(country_name) = %s "
        "ORDER BY population DESC")

    self.city_and_country_name_query = (
        "SELECT ${FUNC}(ST_Force3DZ(the_geom)) AS the_geom, population, city,"
        "country_name, subnational_name, GeometryType(the_geom) AS geom_type "
        "FROM ${CITY_VIEW} "
        "WHERE "
        "lower(city) = %s AND "
        "lower(country_name) = %s")

    self.city_and_country_code_query = (
        "SELECT ${FUNC}(ST_Force3DZ(the_geom)) AS the_geom, population, city,"
        "country_name, subnational_name, GeometryType(the_geom) AS geom_type "
        "FROM ${CITY_VIEW} "
        "WHERE "
        "lower(city) = %s AND lower(country_code) = %s")

    self.city_and_subnational_name_query = (
        "SELECT ${FUNC}(ST_Force3DZ(the_geom)) AS the_geom,"
        "population, city, country_name, subnational_name, "
        "GeometryType(the_geom) AS geom_type "
        "FROM ${CITY_VIEW} "
        "WHERE "
        "lower(city) = %s AND lower(subnational_name) = %s")

    self.city_and_subnational_code_query = (
        "SELECT ${FUNC}(ST_Force3DZ(the_geom)) AS the_geom, "
        "population, city, country_name, subnational_name, "
        "GeometryType(the_geom) AS geom_type "
        "FROM ${CITY_VIEW} "
        "WHERE "
        "lower(city) = %s AND lower(subnational_code) = %s")

    self.example_query = (
        "SELECT ${FUNC}(ST_Force3DZ(the_geom)) AS the_geom,ST_Area(the_geom),"
        "ST_Perimeter(the_geom),sfar_distr,nbrhood, "
        "GeometryType(the_geom) AS geom_type "
        "FROM san_francisco_neighborhoods "
        "WHERE "
        "lower(nbrhood) like %s"
        )

    self.poi_info_by_host_db_name_query = (
        "SELECT A.query_str, A.num_fields "
        "FROM poi_table A JOIN db_poi_table B "
        "ON "
        "A.poi_id = B.poi_id "
        "JOIN db_table C "
        "ON "
        "B.db_id = C.db_id "
        "WHERE "
        "C.host_name = %s "
        "AND "
        "C.db_name = %s"
        )

    self.host_db_name_by_target_query = (
        "SELECT C.host_name, C.db_name "
        "FROM target_table A JOIN target_db_table B "
        "ON "
        "A.target_id = B.target_id "
        "JOIN db_table C "
        "ON "
        "B.db_id = C.db_id "
        "WHERE "
        "A.target_path = %s"
        )


def main():
  constants = Constants()
  port = constants.port
  print port


if __name__ == "__main__":
  main()
