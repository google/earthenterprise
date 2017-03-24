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

//
// Unit tests for ServerdbReader.

#include <string>
#include <vector>
#include "common/serverdb/serverdbReader.h"
#include "common/khStringUtils.h"
#include "common/UnitTest.h"

class ServerdbReader_Test: public UnitTest<ServerdbReader_Test> {
 public:


  // Find all the "pieces" that are part of the request:
  // Depending on GE client version language can be just one piece ("it") or
  // two pieces ("it_IT") and region can either be empty or something like "us"
  // depending on where the GE client is running.
  // We expect request of the form:
  //    language = "it", region = "IT"
  // or language = "it-IT", region = ""
  // or language = "it-IT", region = "us"
  // We would not expect:
  //    language = "it-IT", region = "IT"
  // Comparisons are perfomed case insensitive.
  bool TestGetLocaleKey() {
    const std::string kDefaultKey = "DEFAULT";
    const std::string kDefaultDbRoot = "<MAP_PREFIX>.DEFAULT";
    bool success = true;
    ServerdbConfig::Map toc_map;
    toc_map[kDefaultKey] = kDefaultDbRoot;
    toc_map["it-IT"] = "<MAP_PREFIX>.it-IT";
    toc_map["en-US"] = "<MAP_PREFIX>.en-US";
    toc_map["en-UK"] = "<MAP_PREFIX>.en-UK";
    toc_map["en"] = "<MAP_PREFIX>.en";
    toc_map["fr_FR"] = "<MAP_PREFIX>.fr_FR";

    // Exact matches
    ServerdbConfig::Map::const_iterator iter = toc_map.begin();
    for (; iter != toc_map.end(); ++iter) {
      std::vector<std::string> tokens;
      std::string delimiter = "-";
      TokenizeString(iter->first, tokens, delimiter);
      if (tokens.size() == 1)
        tokens.push_back("");
      EXPECT_EQ(iter->second,
                ServerdbReader::GetLocaleKey(toc_map, tokens[0], tokens[1]));
      EXPECT_EQ(iter->second,
                ServerdbReader::GetLocaleKey(toc_map, iter->first, ""));
      std::string underscore_key = iter->first;
      ReplaceCharsInString(underscore_key, "-", "_");
      EXPECT_EQ(iter->second,
                ServerdbReader::GetLocaleKey(toc_map, underscore_key, ""));
    }
    // best matches
    EXPECT_EQ("<MAP_PREFIX>.en",
              ServerdbReader::GetLocaleKey(toc_map, "en", "AU"));
    EXPECT_EQ("<MAP_PREFIX>.it-IT",
              ServerdbReader::GetLocaleKey(toc_map, "it", "XX"));
    EXPECT_EQ("<MAP_PREFIX>.it-IT",
              ServerdbReader::GetLocaleKey(toc_map, "it-IT", "XX"));
    EXPECT_EQ("<MAP_PREFIX>.it-IT",
              ServerdbReader::GetLocaleKey(toc_map, "it-IT", "IT"));
    EXPECT_EQ("<MAP_PREFIX>.it-IT",
              ServerdbReader::GetLocaleKey(toc_map, "it_IT", ""));
    // Case insensitivity
    EXPECT_EQ("<MAP_PREFIX>.en-US",
              ServerdbReader::GetLocaleKey(toc_map, "en", "us"));
    EXPECT_EQ("<MAP_PREFIX>.en-US",
              ServerdbReader::GetLocaleKey(toc_map, "EN", "US"));
    EXPECT_EQ("<MAP_PREFIX>.en",
              ServerdbReader::GetLocaleKey(toc_map, "EN", "XX"));
    // Also check "_" delimiter
    EXPECT_EQ("<MAP_PREFIX>.it-IT",
              ServerdbReader::GetLocaleKey(toc_map, "it_IT", "XX"));
    EXPECT_EQ("<MAP_PREFIX>.it-IT",
              ServerdbReader::GetLocaleKey(toc_map, "it_IT", "IT"));
    EXPECT_EQ("<MAP_PREFIX>.fr_FR",
              ServerdbReader::GetLocaleKey(toc_map, "fr-FR", "XX"));
    EXPECT_EQ("<MAP_PREFIX>.fr_FR",
              ServerdbReader::GetLocaleKey(toc_map, "fr-FR", "FR"));
    // Tricky
    EXPECT_EQ("<MAP_PREFIX>.en-US",
              ServerdbReader::GetLocaleKey(toc_map, "en_US", "UK"));
    // Invalid requests get the default
    EXPECT_EQ(kDefaultDbRoot,
              ServerdbReader::GetLocaleKey(toc_map, "us", "en"));
    EXPECT_EQ(kDefaultDbRoot,
              ServerdbReader::GetLocaleKey(toc_map, "xx", ""));
    EXPECT_EQ(kDefaultDbRoot,
              ServerdbReader::GetLocaleKey(toc_map, "xx", "yy"));
    EXPECT_EQ(kDefaultDbRoot,
              ServerdbReader::GetLocaleKey(toc_map, "", "yy"));
    return success;
  }

  bool TestGetLocaleKeyMiss() {
    ServerdbConfig::Map toc_map;
    toc_map["it-IT"] = "<MAP_PREFIX>.it-IT";

    // best matches
    EXPECT_EQ(std::string(),
              ServerdbReader::GetLocaleKey(toc_map, "en", "AU"));

    return true;
  }

  ServerdbReader_Test(void) : BaseClass("ServerdbReader_Test") {
    REGISTER(TestGetLocaleKey);
    REGISTER(TestGetLocaleKeyMiss);
  }
};

int main(int argc, char *argv[]) {
  ServerdbReader_Test tests;

  return tests.Run();
}
