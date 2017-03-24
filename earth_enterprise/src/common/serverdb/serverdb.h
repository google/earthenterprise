/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef COMMON_SERVERDB_SERVERDB_H__
#define COMMON_SERVERDB_SERVERDB_H__

#include <string>
#include <map>
#include <vector>

class ServerdbConfig
{
 public:
  typedef std::map<std::string, std::string> Map;
  enum DbType {TYPE_GEDB = 0, TYPE_MAPDB = 1};
  DbType db_type;
  std::string index_path;
  Map toc_paths;
  std::vector<std::string> icons;
  std::string searchtabs_path;
  Map json_paths;

  bool Load(std::string config_file);
  bool Save(std::string config_file);
};

#endif /* COMMON_SERVERDB_SERVERDB_H__ */
