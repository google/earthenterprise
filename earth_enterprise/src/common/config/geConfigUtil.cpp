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

#include "geConfigUtil.h"

#include <fstream>
#include <vector>

#include <khException.h>
#include <khSpawn.h>
#include <khnet/InetAddr.h>
#include <khstl.h>
#include <config/gePrompt.h>
#include <khFileUtils.h>
#include <khStringUtils.h>
#include <khSimpleException.h>


void AssertRunningAsRoot(void) {
  // uid 0 -> root
  if (::geteuid() != 0) {
    throw khException(kh::tr("You must run as root"));
  }
}

bool IsRedHat(void) {
  return khExists("/etc/redhat-release");
}

uint GetNumCPUs(void) {
  uint numcpus = 0;
  std::ifstream in("/proc/cpuinfo");
  if (in) {
    std::string line;
    while (std::getline(in, line)) {
      if (StartsWith(line, "processor")) {
        ++numcpus;
      }
    }
  }

  return std::max(uint(1), numcpus);
}

uint64 GetPhysicalMemorySize(void) {
  std::ifstream in("/proc/meminfo");
  const std::string delimiters(" \t");
  if (in) {
    std::string line;
    while (std::getline(in, line)) {
      // will be a line of the form: MemTotal:       255944 kB
      if (StartsWith(line, "MemTotal")) {
        // parse out the kB.
        std::vector<std::string> tokens;
        TokenizeString(line, tokens, delimiters);
        if (tokens.size() == 3 && tokens[2] == "kB") {
          return strtouq(tokens[1].c_str(), NULL, 10) * 1000;
        }
        return 0;  // parse failure...too many tokens or not "kB"
      }
    }
    return 0;  // parse failure
  }

  return 0;  // parse failure
}

uint GetMaxFds(int requested) {
  int systemMax = khMaxOpenFiles();
  if (requested > 0) {
    return std::min(systemMax, requested);
  } else {
    return std::max(1, systemMax+requested);
  }
}


std::string GetAndValidateHostname(void) {
  std::string hostname = khHostname();

  if (hostname.empty() ||
      (hostname == "(none)") ||
      (hostname == "localhost")) {
    throw khException(
        kh::tr("This machine's hostname has not been configured.\n"
               "Please use the system utilities to define a hostname."));
  }

  // try to see if we can get an InetAddr from the given hostname
  InetAddr addr(hostname);
  if (!addr) {
    throw khException(
        kh::tr(
            "This machine's hostname (%1) does not map to an IP address.\n",
            "Please use the system utilities to reconfigure the hostname.")
        .arg(hostname));
  }

  return hostname;
}

// For Server-only installs, there is no systemrc file, instead usernames
// are stored in gevars.sh.
std::string kServerOnlyUserNamesFile = "/etc/init.d/gevars.sh";
std::string kServerOnlyApacheUserKey = "GEAPACHEUSER";
std::string kServerOnlyFusionUserKey = "GEFUSIONUSER";
std::string kServerOnlyUserGroupnameKey = "GEGROUP";

std::string ApacheUserNameServerOnly() {
  std::string name(FindValueInVariableFile(kServerOnlyUserNamesFile,
                                    kServerOnlyApacheUserKey));
  if (name.empty()) {
    throw khSimpleException("Error retrieving the Apache User Name from ")
      << kServerOnlyUserNamesFile;
  }
  return name;
}
std::string FusionUserNameServerOnly() {
  std::string name(FindValueInVariableFile(kServerOnlyUserNamesFile,
                                    kServerOnlyFusionUserKey));
  if (name.empty()) {
    throw khSimpleException("Error retrieving the Fusion User Name from ")
      << kServerOnlyUserNamesFile;
  }
  return name;
}
std::string UserGroupnameServerOnly() {
  std::string name(FindValueInVariableFile(kServerOnlyUserNamesFile,
                                    kServerOnlyUserGroupnameKey));
  if (name.empty()) {
    throw khSimpleException("Error retrieving the Fusion User Name from ")
      << kServerOnlyUserNamesFile;
  }
  return name;
}


