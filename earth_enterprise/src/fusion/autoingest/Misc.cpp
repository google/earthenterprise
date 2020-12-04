// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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



#include "Misc.h"
#include <time.h>
#include "Asset.h"
#include "AssetVersion.h"
#include <khStringUtils.h>
#include <khstl.h>
#include <khException.h>

std::string
GetFormattedTimeString(time_t timeval)
{
  std::string format = "%F %T";
  return GetTimeStringWithFormat(timeval, format);
}

std::string
GetFormattedElapsedTimeString(unsigned int elapsed)
{
  char pretty[256];
  char buf[400];

  if (elapsed >= 60) {
    std::uint32_t sec  = elapsed;
    std::uint32_t hour = sec/3600;
    sec -= hour*3600;
    std::uint32_t min = sec/60;
    sec -= min*60;
    if (hour > 0) {
      snprintf(pretty, sizeof(pretty),
               " (%uh%um%us)", hour, min, sec);
    } else {
      snprintf(pretty, sizeof(pretty),
               " (%um%us)", min, sec);
    }
  } else {
    pretty[0] = 0;
  }
  snprintf(buf, sizeof(buf),
           "%u seconds%s", elapsed, pretty);
  return buf;
}



void
GetMinMaxLevels(const std::string &ref, unsigned int &minlevel, unsigned int &maxlevel)
{
  if (AssetVersionRef(ref).Version() == "current") {
    Asset asset(ref);
    minlevel = asset->meta.GetAs< unsigned int> ("minlevel");
    maxlevel = asset->meta.GetAs< unsigned int> ("maxlevel");
  } else {
    AssetVersion version(ref);
    minlevel = version->meta.GetAs< unsigned int> ("minlevel");
    maxlevel = version->meta.GetAs< unsigned int> ("maxlevel");
  }
}

std::string
varsubst(const std::string &str,
         const std::string &varname,
         const std::string &subst,
         const SubstQualMap &qualifiers)
{
  std::string result = str;

  if (qualifiers.empty()) {
    std::string tofind = "${" + varname + "}";
    std::string::size_type pos = 0;
    while ((pos = result.find(tofind, pos)) != std::string::npos) {
      result.replace(pos, tofind.size(), subst);
      pos += subst.size();
    }
  } else {
    std::string tofind = "${" + varname;
    std::string::size_type pos = 0;
    while ((pos = result.find(tofind, pos)) != std::string::npos) {
      std::string::size_type qualpos = pos + tofind.size();
      std::string::size_type end = result.find("}", qualpos);
      if (end != std::string::npos) {
        if (result[qualpos] == '}') {
          result.replace(pos, end - pos + 1, subst);
          pos += subst.size();
        } else if (result[qualpos] == ':') {
          std::string tosubst = subst;
          std::vector<std::string> qualstrs;
          split(result.substr(qualpos+1, end - (qualpos+1)), ":",
                std::back_inserter(qualstrs));
          for(const auto& q : qualstrs) {
            SubstQualMap::const_iterator found =
              qualifiers.find(q);
            if (found != qualifiers.end()) {
              tosubst = (*found->second)(tosubst);
            } else {
              throw khException
                (kh::tr("Unrecognized qualifier: ").toUtf8().constData() + q);
            }
          }
          result.replace(pos, end - pos + 1, tosubst);
          pos += tosubst.size();
        } else {
          // this variable isn't for me, skip the part we matched
          pos = qualpos;
        }
      } else {
        // this variable isn't for me, just skip the part we matched
        pos = qualpos;
      }

    }
  }
  return result;
}
