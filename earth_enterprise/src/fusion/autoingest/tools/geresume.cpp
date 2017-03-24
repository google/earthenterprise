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
#include <notify.h>
#include <khGetopt.h>
#include <khstl.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>


// ****************************************************************************
// ***  This program used used for multiple purposes to manipulate
// ***  asset versions
// ***
// ***  gerestart
// ***  gecancel
// ***  gesetbad
// ***  geclearbad
// ***  geclean
// ****************************************************************************

void
usage(const char *prog, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] <assetname> [<version>]\n"
          "   <version> can be a number, 'current', or 'lastgood'\n"
          "      if ommited <assetname> is checked for '?version=...'\n"
          "   Supported options are:\n"
          "      --help | -?:    Display this usage message\n",
          prog);
  exit(1);
}


int
main(int argc, char *argv[]) {

  try {
    // process commandline options
    int argn;
    bool help = false;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    if (!options.processAll(argc, argv, argn)) {
      usage(argv[0]);
    }
    if (help) {
      usage(argv[0]);
    }

    // process rest of commandline arguments
    const char *assetname = GetNextArg();
    const char *version = GetNextArg();
    if (!assetname) {
      usage(argv[0], "<assetname> not specified");
    }
    if (!version) version = "";

    std::string versionref
      = AssetDefs::GuessAssetVersionRef(assetname, version);


    // figure out which cmd to run
    bool (*cmd)(const std::string &, QString&, int) = 0;;
    std::string progname = argv[0];
    if (EndsWith(progname, "resume"))
      cmd = &khAssetManagerProxy::RebuildVersion;
    else if (EndsWith(progname, "cancel"))
      cmd = &khAssetManagerProxy::CancelVersion;
    else if (EndsWith(progname, "setbad"))
      cmd = &khAssetManagerProxy::SetBadVersion;
    else if (EndsWith(progname, "clearbad"))
      cmd = &khAssetManagerProxy::ClearBadVersion;
    else if (EndsWith(progname, "clean"))
      cmd = &khAssetManagerProxy::CleanVersion;
    else {
      usage(argv[0], "Internal Error: Don't know which command to run.");
    }


    // now send the request
    QString error;
    if (!(*cmd)(versionref, error, 0 /* timeout */)) {
      notify(NFY_FATAL, "%s", error.latin1());
    }
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
