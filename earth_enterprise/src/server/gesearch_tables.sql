--
-- Copyright 2018 Google Inc.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--

SET CLIENT_MIN_MESSAGES TO WARNING;

--Check if the table already exists.
--Return TRUE/FALSE based on whether tables exists or not.
CREATE OR REPLACE FUNCTION check_table_exists(table_name varchar)
RETURNS BOOLEAN AS $$
BEGIN

  IF EXISTS(SELECT 1 FROM pg_tables WHERE tablename = table_name) THEN
    RETURN TRUE;
  ELSE
    RETURN FALSE;
  END IF;

END;
$$ LANGUAGE plpgsql;

--create tables 'db_table','poi_table','db_poi_table'
--and 'search_def_table' if they don't exist already.

CREATE OR REPLACE FUNCTION create_tables()
RETURNS VOID AS $$
BEGIN

  IF NOT check_table_exists('db_table') THEN
    CREATE TABLE db_table  (
      db_id serial,
      host_name varchar(150) not null,
      db_name varchar(300) not null,
      db_pretty_name varchar(300) not null
    );
    alter table db_table add primary key (host_name, db_name);
    RAISE INFO 'TABLE % CREATED', 'db_table';
  END IF;

  IF check_table_exists('poi_table') THEN
    IF EXISTS(select column_name from information_schema.columns where table_name = 'poi_table' and column_name = 'poi_file_size' and data_type = 'integer') THEN
      alter table poi_table alter column poi_file_size set data type bigint;
    END IF;
  ELSE
    create table poi_table (
      poi_id serial,
      query_str varchar(500) not null,
      host_name varchar(150) not null,
      poi_file_path varchar(300) not null,
      poi_file_size bigint not null,
      style varchar(500) not null,
      num_fields integer not null,
      status integer not null
    );
    alter table poi_table add primary key (host_name, poi_file_path);
    RAISE INFO 'TABLE % CREATED', 'poi_table';
  END IF;

  IF NOT check_table_exists('db_poi_table') THEN
    create table db_poi_table (
      db_id integer not null,
      poi_id integer not null
    );
    alter table db_poi_table add primary key (db_id, poi_id);
    RAISE INFO 'TABLE % CREATED', 'db_poi_table';
  END IF;

  IF NOT check_table_exists('search_def_table') THEN
    create table search_def_table (
      search_def_id serial,
      search_def_name varchar(150) not null,   -- search definition identifier/nickname.
      search_def_content text not null
    );
    alter table search_def_table add primary key (search_def_name);
    RAISE INFO 'TABLE % CREATED', 'search_def_table';
  END IF;

END;
$$ LANGUAGE plpgsql;


-- search_def_content is
--   search_def_label,      --label for search def block in html page.
--   service_url,           --search service path/URL.
--   search_fields,         --search fields: (label, suggestion, key) tuples.
--                          --filed label is empty for all one-field searches.
--   additional_query_param,
--   additional_config_param
--   html_transform_url
--   kml_transform_url
--   result_type
--   suggest_server

--upsert_searchdef_content() performs either an insert or an update of the
--'search_def_table' table in 'gesearch' database based on whether
--the search_name already exists or not.

--In case the search_name already exists, a unique_violation exception occurs
--in which case an update statement is run.


CREATE OR REPLACE FUNCTION upsert_searchdef_content(search_name varchar, search_content varchar)
--search_name - This is the search definition name.
--search_content - This is the search definition content for search_name.

RETURNS VOID AS $$
BEGIN
  INSERT INTO search_def_table(search_def_name, search_def_content) values(search_name, search_content);
  EXCEPTION WHEN unique_violation THEN
    UPDATE search_def_table SET search_def_content = search_content WHERE search_def_name = search_name;
END;
$$ LANGUAGE plpgsql;


--Function to trigger the 'upsert_searchdef_content' function.
--Any changes to the existing search services or
--any new system search services should be added here.

--run_searchdef_upsert() iterates through an array of search definition names
--and triggers the upsert_searchdef_content() passing the search definition name
--and the search definition content to be inserted/updated in the
--'search_def_table' table in the 'gesearch' database.

CREATE OR REPLACE FUNCTION run_searchdef_upsert()
RETURNS VOID AS $$
DECLARE
   searchDefNames VARCHAR[6] := ARRAY['POISearch', 'GeocodingFederated', 'Coordinate', 'Places', 'SearchGoogle'];
   searchDefContents VARCHAR[6] := ARRAY[
            '{"additional_config_param": null, "label": "POI", "additional_query_param": "flyToFirstElement=true&displayKeys=location", "service_url": "POISearch", "result_type": "KML", "html_transform_url": "about:blank", "kml_transform_url":  "about:blank", "suggest_server": "about:blank", "fields": [{"key": "q", "suggestion": "Point of interest", "label": null}]}',
            '{"additional_config_param": null, "label": "Places or coordinates", "additional_query_param": "flyToFirstElement=true&displayKeys=location", "service_url": "/gesearch/FederatedSearch", "result_type": "KML", "html_transform_url": "about:blank", "kml_transform_url":  "about:blank", "suggest_server": "about:blank", "fields": [{"key": "q", "suggestion": "City, country or lat, lng (e.g., 23.47, -132.67)", "label": null}]}',
            '{"additional_config_param": null, "label": "Coordinates", "additional_query_param": "flyToFirstElement=true&displayKeys=location", "service_url": "/gesearch/CoordinateSearch", "result_type": "KML", "html_transform_url": "about:blank", "kml_transform_url":  "about:blank", "suggest_server": "about:blank", "fields": [{"key": "q", "suggestion": "Latitude, longitude (e.g., 23.47, -132.67)", "label": null}]}',
            '{"additional_config_param": null, "label": "Places", "additional_query_param": "flyToFirstElement=true&displayKeys=location", "service_url": "/gesearch/PlacesSearch", "result_type": "KML", "html_transform_url": "about:blank", "kml_transform_url":  "about:blank", "suggest_server": "about:blank", "fields": [{"key": "q", "suggestion": "City, country", "label": null}]}',
            '{"additional_config_param": null, "label": "Search Google", "additional_query_param": "flyToFirstElement=true&displayKeys=location", "service_url": "/gesearch/SearchGoogle", "result_type": "XML", "html_transform_url": "https://www.google.com/earth/client/unifiedsearch/syndication_to_html_$[hl].xsl", "kml_transform_url":  "https://www.google.com/earth/client/unifiedsearch/syndication_to_kml_$[hl].xsl", "suggest_server": "http://maps.google.com/maps/suggest", "fields": [{"key": "q", "suggestion": "5520 Quebec Place Washington", "label": null}]}'
            ];

BEGIN
 FOR i IN 1..array_length(searchDefNames,1) LOOP
  PERFORM upsert_searchdef_content(searchDefNames[i], searchDefContents[i]);
 END LOOP;
END;
$$ LANGUAGE plpgsql;


--create tables.
SELECT create_tables();

--insert/update system search services.
SELECT run_searchdef_upsert();
