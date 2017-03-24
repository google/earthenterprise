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



#include <stdio.h>
#include <khGetopt.h>
#include <autoingest/.idl/Systemrc.h>
#include <config/geConfigUtil.h>
#include <config/gefConfigUtil.h>
#include <config/AssetRootStatus.h>
#include <geUsers.h>

void
usage(const char* prog, const char* msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(
stderr,
"\n"
"usage: %s --checkassetroot\n"
"       %s --checksysman\n"
"       %s --checkrunning\n"
"       --username <Fusion username>\n"
"       --groupname <Fusion group name>\n"
"Check if fusion daemons can run.\n"
"Supported options are:\n"
"  --help, -?:        Displays this help message\n"
"  --checkassetroot:  Make sure the selected asset root is OK\n"
"  --checksysman:     Check if this host should run gesystemmanager\n",
"  --checkrunning:    Check if the daemons are running\n",
prog, prog);
  exit(1);
}



int
main(int argc, char *argv[]) {
  try {
    bool help = false;
    bool checkassetroot = false;
    bool checksysman    = false;
    bool checkrunning   = false;
    std::string username = Systemrc::FusionUsername();
    std::string groupname = Systemrc::UserGroupname();
    int argn;

    khGetopt options;
    options.helpOpt(help);
    options.opt("checkassetroot", checkassetroot);
    options.opt("checksysman", checksysman);
    options.opt("checkrunning", checkrunning);
    options.opt("username", username);
    options.opt("groupname", groupname);
    options.setExclusiveRequired("checkassetroot", "checksysman",
                                 "checkrunning");

    if (!options.processAll(argc, argv, argn) || help) {
      usage(argv[0]);
    }

    // we cannot call ValidateHostForConfig since that asserts
    // that fusion is not running and we need to call this while fusion is
    // running to know what to stop.
    AssertRunningAsRoot();
    SwitchToUser(username, groupname);

    if (checkrunning) {
      exit(IsFusionRunning() ? 0 : 1);
    }

    std::string thishost = GetAndValidateHostname();
    AssetRootStatus status(Systemrc::TryAssetRoot(), thishost,
                           username, groupname);

    if (checksysman) {
      exit(status.IsThisMachineMaster() ? 0 : 1);
    } else if (checkassetroot) {
      status.ValidateForDaemonCheck();
    }

    return 0;
  } catch (const std::exception &e) {
    fprintf(stderr, "\n%s\n", e.what());
  } catch (...) {
    fprintf(stderr, "Unknown error\n");
  }
  return 1;
}
