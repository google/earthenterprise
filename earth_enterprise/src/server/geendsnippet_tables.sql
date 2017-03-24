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

SET CLIENT_MIN_MESSAGES = WARNING;

-- User snippets sets (snippets profiles).
create table snippet_set_table (
  name varchar(150) not null primary key,  -- id/nickname.
  content text not null        -- snippets set (proto dbroot end_snippet in
                               -- internal format serialized to json string).
);

