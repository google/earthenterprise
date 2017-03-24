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

#ifndef __Misc_h
#define __Misc_h

#include <string>
#include <map>
#include <time.h>
#include <common/khTypes.h>

extern std::string GetFormattedTimeString(time_t timeval = 0);
extern std::string GetFormattedElapsedTimeString(uint elapsed);
extern void GetMinMaxLevels(const std::string &ref,
                            uint &minlevel, uint &maxlevel);

typedef std::map<std::string,std::string(*)(const std::string &)> SubstQualMap;
// this can throw
extern std::string varsubst(const std::string &str,
                            const std::string &varname,
                            const std::string &subst,
                            const SubstQualMap &qualifiers = SubstQualMap());

extern const char * const PluginNames[];
extern unsigned int NumPlugins;

template <class Inset>
inline uint ComputeEffectiveMaxLevel(const Inset &inset)
{
  uint insetmin = 0;
  uint insetmax = 0;
  GetMinMaxLevels(inset.dataAsset, insetmin, insetmax);

  uint effectivemax = inset.overridemax;
  if (!effectivemax) {
    effectivemax = insetmax;
  } else if (effectivemax < insetmin) {
    effectivemax = insetmin;
  } else if (effectivemax > insetmax) {
    effectivemax = insetmax;
  }
  return effectivemax;
}



#endif /* __Misc_h */
