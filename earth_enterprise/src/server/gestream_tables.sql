-- Copyright 2018 Google Inc.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--      http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--


SET CLIENT_MIN_MESSAGES = WARNING;

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

CREATE OR REPLACE FUNCTION create_or_update_tables()
RETURNS VOID AS $$
BEGIN
  IF check_table_exists('db_table') THEN
    IF NOT EXISTS(select column_name from information_schema.columns where table_name = 'db_table' and column_name = 'db_flags') THEN
      alter table db_table add column db_flags integer not null default 0;
    END IF;
  ELSE
    create table db_table (
      db_id serial,
      host_name varchar(150) not null,
      db_name varchar(300) not null,
      db_pretty_name varchar(300) not null,
      db_timestamp timestamp with time zone null,
      db_size bigint null,
      db_flags integer not null default 0
    );
    alter table db_table add primary key (host_name, db_name);
    CREATE INDEX db_table_db_id_idx ON db_table(db_id);
  END IF;

  IF NOT check_table_exists('files_table') THEN
    create table files_table (
      file_id serial,
      host_name varchar(150) not null,
      file_path varchar(300) not null,
      file_size bigint not null,
      file_status integer not null
    );
    alter table files_table add primary key (host_name, file_path);
    CREATE INDEX files_table_file_id_idx ON files_table(file_id);
  END IF;

  IF NOT check_table_exists('db_files_table') THEN
    create table db_files_table (
      db_id integer not null,
      file_id integer not null
    );
    alter table db_files_table add primary key(db_id, file_id);
    CREATE INDEX db_files_table_file_id_idx ON db_files_table(file_id);
  END IF;

  IF check_table_exists('virtual_host_table') THEN
    IF NOT EXISTS(select column_name from information_schema.columns where table_name = 'virtual_host_table' and column_name = 'virtual_host_ssl') THEN
      alter table virtual_host_table add column virtual_host_ssl bool not null default false;
    END IF;
    IF NOT EXISTS(select virtual_host_name from virtual_host_table where virtual_host_name = 'local') THEN
      insert into virtual_host_table(virtual_host_name, virtual_host_url, virtual_host_cache_level) values('local', '/local_host', 2);
    END IF;
  ELSE
    create table virtual_host_table (
      virtual_host_id serial,
      virtual_host_name varchar(150) not null,
      virtual_host_url varchar(300) not null,
      virtual_host_cache_level integer not null,
      virtual_host_ssl bool not null default false
    );
    alter table virtual_host_table add primary key (virtual_host_name);
    insert into virtual_host_table(virtual_host_name, virtual_host_url, virtual_host_cache_level) values('local', '/local_host', 2);
    insert into virtual_host_table(virtual_host_name, virtual_host_url, virtual_host_cache_level) values('public', '/public_host', 2);
    insert into virtual_host_table(virtual_host_name, virtual_host_url, virtual_host_cache_level) values('secure', '/secure_host', 2);
  END IF;

  IF NOT check_table_exists('target_table') THEN
    create table target_table (
      target_id serial,
      target_path varchar(300) not null,
      serve_wms bool not null default false
    );
    alter table target_table add primary key (target_path);
  END IF;

  IF check_table_exists('target_db_table') THEN
    IF NOT EXISTS(select column_name from information_schema.columns where table_name = 'target_db_table' and column_name = 'publish_context_id') THEN
      alter table target_db_table add column publish_context_id integer not null default 0;
    END IF;
  ELSE
    create table target_db_table (
      target_id integer not null,
      virtual_host_id integer not null,
      db_id integer not null,
      publish_context_id integer not null default 0
    );
    alter table target_db_table add primary key (target_id, db_id);
  END IF;


  IF check_table_exists('publish_context_table') THEN
    IF NOT EXISTS(select column_name from information_schema.columns where table_name = 'publish_context_table' and column_name = 'ec_default_db') THEN
      alter table publish_context_table add column ec_default_db boolean not null default false;
    END IF;
  ELSE 
    create table publish_context_table (
      publish_context_id serial,
      snippets_set_name varchar(150),
      search_def_names varchar(150)[] default '{}',
      supplemental_search_def_names varchar(150)[] default '{}',
      poifederated boolean not null default false,
      ec_default_db boolean not null default false
    );
  END IF;

  IF NOT check_table_exists('cut_spec_table') THEN
    create table cut_spec_table (
      cut_spec_id serial,
      name varchar(150) not null,
      qtnodes text,
      exclusion_qtnodes text,
      min_level integer,
      default_level integer,
      max_level integer
    );
  END IF;

END;
$$ LANGUAGE plpgsql;

SELECT create_or_update_tables();
