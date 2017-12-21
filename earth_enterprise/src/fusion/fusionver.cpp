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

#include "fusionversion.h"
#include <gee_version.h>
#include <khFileUtils.h>
#include <geInstallPaths.h>
#include <khGetopt.h>



namespace {
bool              FusionTypeSet = false;
FusionProductType FusionType = FusionLT;
}

void SetFusionProductType(FusionProductType type) {
  FusionType = type;
  FusionTypeSet = true;
}



FusionProductType GetFusionProductType(void) {
  if (!FusionTypeSet) {
    std::string progname = khGetopt::GetFullProgramName();
    std::string progdir = khDirname(progname);

    if (khExists(khComposePath(progdir, "scroll")) ||
        khExists(khComposePath(kGEBinPath, "scroll"))) {
      SetFusionProductType(FusionInternal);
    } else if (khExists(khComposePath(progdir, "gerasterimport")) ||
               khExists(khComposePath(kGEBinPath, "gerasterimport"))) {
      SetFusionProductType(FusionPro);
    } else {
      SetFusionProductType(FusionLT);
    }
  }

  return FusionType;
}

extern std::string GetFusionProductName(void) {
  switch (GetFusionProductType()) {
    case FusionLT:
      return "Google Earth Fusion LT";
    case FusionPro:
      return "Google Earth Fusion Pro";
    case FusionInternal:
      return "Google Earth Fusion Internal";
  }

  // unreached but silences warnings
  return std::string();
}

extern std::string GetFusionProductShortName(void) {
  switch (GetFusionProductType()) {
    case FusionLT:
      return "Fusion LT";
    case FusionPro:
      return "Fusion Pro";
    case FusionInternal:
      return "Fusion Internal";
  }

  // unreached but silences warnings
  return std::string();
}
