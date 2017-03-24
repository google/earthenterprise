// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <string>
#include <map>
#include <vector>
#include <stdio.h>

#include "common/khGuard.h"
#include "common/khStringUtils.h"
#include "common/khFileUtils.h"
#include "common/notify.h"
#include "common/khConstants.h"
#include "common/serverdb/serverdb.h"

namespace {
const std::string kDbType = "<db_type>";
const std::string kIndexPath = "<index_path>";
const std::string kTocPath = "<toc_path>";
const std::string kIcon = "<icon>";
const std::string kSearchTabsPathTag = "<searchtab_path>";
const std::string kJsonPath = "<json_path>";
const std::string kDelim = "|";
}  // end namespace

bool ServerdbConfig::Load(std::string config_file) {
  FILE* fp = fopen(config_file.c_str(), "r");
  if (fp == NULL) {
    notify(NFY_WARN, "Unable to open ServerDB config file %s",
           config_file.c_str());
    return false;
  }
  khFILECloser closer(fp);  // guarantee the file will get closed

  // Set the default db type to gedb.
  db_type = TYPE_GEDB;

  while (!feof(fp)) {
    const int kTempBufSize = 8192;
    char buf[kTempBufSize];
    buf[0] = 0;
    fgets(buf, kTempBufSize, fp);
    std::string line(buf);
    CleanString(&line, "\r\n");
    std::vector<std::string> tokens;
    TokenizeString(line, tokens, kDelim, 3);
    if (tokens.size() < 2)
      continue;

    if (tokens[0] == kDbType) {
      db_type = static_cast<DbType>(atoi(tokens[1].c_str()));
    } else if (tokens[0] == kIndexPath) {
      index_path = tokens[1];
    } else if (tokens[0] == kTocPath && tokens.size() == 3) {
       toc_paths[tokens[1]] = tokens[2];
    } else if (tokens[0] == kJsonPath && tokens.size() == 3) {
       json_paths[tokens[1]] = tokens[2];
    } else if (tokens[0] == kIcon) {
      icons.push_back(tokens[1]);
    } else if (tokens[0] == kSearchTabsPathTag) {
      searchtabs_path = tokens[1];
    }
  }

  // Make sure we loaded everything correctly.
  if (index_path.empty() || toc_paths.size() == 0) {
    notify(NFY_WARN, "Unable to parse ServerDB config file %s",
           config_file.c_str());
    return false;
  }

  // If index_path is DB relative, convert to absoulte path
  if (khIsDbSuffix(index_path, kUnifiedIndexKey)) {
    std::string db_prefix = khGetDbPrefix(
        config_file, db_type == TYPE_GEDB ? kGedbKey : kMapdbKey);
    index_path = db_prefix + index_path;
  }

  return true;
}


bool ServerdbConfig::Save(std::string config_file) {
  FILE* fp = fopen(config_file.c_str(), "w");
  if (fp == NULL) {
    notify(NFY_WARN, "Unable to save ServerDB config file %s",
           config_file.c_str());
    return false;
  }
  khFILECloser closer(fp);

  fprintf(fp, "%s%s%d\n", kDbType.c_str(), kDelim.c_str(), db_type);
  const std::string relative_index_path = khGetDbSuffix(
      index_path, kUnifiedIndexKey);
  fprintf(fp, "%s%s%s\n", kIndexPath.c_str(), kDelim.c_str(),
          relative_index_path.c_str());
  Map::const_iterator it = toc_paths.begin();
  for (; it != toc_paths.end(); ++it)
    fprintf(fp, "%s%s%s%s%s\n", kTocPath.c_str(), kDelim.c_str(),
                it->first.c_str(), kDelim.c_str(), it->second.c_str());

  std::vector<std::string>::const_iterator icon = icons.begin();
  for (; icon != icons.end(); ++icon)
    fprintf(fp, "%s%s%s\n", kIcon.c_str(), kDelim.c_str(), icon->c_str());

  fprintf(fp, "%s%s%s\n", kSearchTabsPathTag.c_str(), kDelim.c_str(),
                          searchtabs_path.c_str());

  // Output the json paths.
  it = json_paths.begin();
  for (; it != json_paths.end(); ++it)
    fprintf(fp, "%s%s%s%s%s\n", kJsonPath.c_str(), kDelim.c_str(),
                it->first.c_str(), kDelim.c_str(), it->second.c_str());

  return true;
}
