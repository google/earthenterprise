--
-- Copyright 2017 Google Inc.
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
-- 3.1 Added indexes to several tables in the gestream db
-- this should be applied to any 3.0.X postgres gestream db's
CREATE INDEX db_table_db_id_idx ON db_table(db_id);
CREATE INDEX files_table_file_id_idx ON files_table(file_id);
CREATE INDEX db_files_table_file_id_idx ON db_files_table(file_id);
