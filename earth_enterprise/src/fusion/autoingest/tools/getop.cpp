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


#include <notify.h>
#include <unistd.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>
#include <khSpawn.h>
#include <khGetopt.h>
#include <fusionversion.h>
#include <config/gefConfigUtil.h>
#include <autoingest/.idl/Systemrc.h>
#include "MiscConfig.h"

// global for convenience
int numcols = 80;

static const std::string SYS_MGR_BUSY_MSG = "GetCurrTasks: " + sysManBusyMsg;

void
outline(const char *format, ...)
{
  char buf[numcols+2];

  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  if (len > 0) {
    if (len >= numcols+1) {
      len = numcols; // leave room for '\n'
    }
    if (buf[len-1] != '\n') {
      buf[len++] = '\n';
    }
    buf[len] = 0;
    fputs(buf, stdout);
  } else {
    fputs("\n", stdout);
  }
}


void
usage(const std::string &progn, const char *msg = 0, ...)
{
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options]\n"
          "   Supported options are:\n"
          "      --help | -?:       Display this usage message\n"
          "      --delay <seconds>: Seconds to wait between refreshes\n"
          "      --timeout <seconds>: Seconds to set as timeout for gesystemmanager repsonses.\n",
          progn.c_str());
  exit(1);
}

// Convert the amount of memory used by caches to a more easily read format
std::string
readableMemorySize(std::uint64_t size) {
  double readable = static_cast<double>(size);
  const std::array<std::string, 9> units = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
  std::uint8_t i = 0;
  std::stringstream memoryUsed;

  while (readable > 1024) {
    readable /= 1024;
    i++;
  }

  memoryUsed << std::fixed;
  memoryUsed.precision(2);
  memoryUsed << readable << ' ' << units[i];
  return memoryUsed.str();
}

int
main(int argc, char *argv[])
{
  try {
    std::string progname = argv[0];
    int argn;
    bool help  = false;
    int delay = 5;
    int timeout = 60;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("delay", delay);
    options.opt("timeout", timeout);
    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);
    if (delay <= 0)
      usage(progname, "--delay must be positive");
    if (timeout < 0)
      usage(progname, "--timeout must not be less than zero");

    std::string master    = AssetDefs::MasterHostName();
    std::string assetroot = AssetDefs::AssetRoot();
    CmdLine clearscreen;
    clearscreen << "clear";
    outline("Connecting to gesystemmanager to retrieve status...");

    while (1) {
      QString error;
      TaskLists taskLists;
      if (!khAssetManagerProxy::GetCurrTasks("dummy", taskLists,
                                             error, timeout)) {
      	if (error.compare("GetCurrTasks: socket recvall: Resource temporarily unavailable") == 0)
          outline("No data received from gesystemmanager\nStarting new request");
        else if (error.compare(SYS_MGR_BUSY_MSG.c_str()) == 0)
          outline("System Manager is busy.  Retrying in %d seconds", delay);
        else
          notify(NFY_FATAL, "%s", error.latin1());
      } else {
        // get the list of active keyhole processes
        std::vector<std::string> pslist;
        Systemrc systemrc;
        LoadSystemrc(systemrc);
        std::string ps_command = "ps -l --user " + systemrc.fusionUsername +
          " --cols 100000 | awk '{ print $4 \"\t    \" $13 \"\t\" $14 }'";
        FILE *psFILE = popen(ps_command.c_str(), "r");
        if (psFILE) {
          char buf[512];
          bool first = true;
          while (fgets(buf, sizeof(buf), psFILE)) {
            if (first) {
              first = false;
            } else {
              char *tmp = buf;
              while (*tmp) {
                if (*tmp == '\t')
                  *tmp = ' ';
                ++tmp;
              }
              pslist.push_back(buf);
            }
          }
          pclose(psFILE);
        }

        // get number of lines that the terminal has
        int numlines = 40;
        FILE *tputFILE = popen("tput lines", "r");
        if (tputFILE) {
          char buf[256];
          if (fgets(buf, sizeof(buf), tputFILE)) {
            numlines = atoi(buf);
          }
          pclose(tputFILE);
        }

        // get number of cols that the terminal has
        tputFILE = popen("tput cols", "r");
        if (tputFILE) {
          char buf[256];
          if (fgets(buf, sizeof(buf), tputFILE)) {
            numcols = atoi(buf);
          }
          pclose(tputFILE);
        }

        // clear screen
        (void)clearscreen.System();

        // emit khtop header
        outline("FUSION VERSION: %s", GEE_VERSION);
        outline("MASTER: %s    ASSET ROOT: %s",
                master.c_str(), assetroot.c_str());
        outline("");
        numlines -= 3;

        // providers & active lists
        for (std::vector<TaskLists::Provider>::const_iterator p =
               taskLists.providers.begin();
             p != taskLists.providers.end(); ++p) {
          outline("Active on %s (max = %d):",
                  p->hostname.c_str(), p->maxjobs);
          --numlines;
          for (std::vector<TaskLists::ActiveTask>::const_iterator a =
                 p->activeTasks.begin();
               a != p->activeTasks.end(); ++a) {
            outline("  %s\n", a->verref.c_str());
            --numlines;
          }

        }

        // waiting list
        outline("Waiting:");
        --numlines;
        for (std::vector<TaskLists::WaitingTask>::const_iterator w =
               taskLists.waitingTasks.begin();
             w != taskLists.waitingTasks.end(); ++w) {
          if (numlines > 6) {
            outline("  %s", w->verref.c_str());
            --numlines;
            if (!w->activationError.isEmpty()) {
              outline("     %s", w->activationError.latin1());
              --numlines;
            }
          } else {
            outline("  AND OTHERS ...");
            --numlines;
            break;
          }
        }

        outline("");
        outline("Fusion processes on this host:");
        numlines -= 6;

        for (unsigned int i = 0; i < pslist.size(); ++i) {
          if (numlines > 2) {
            outline("%s", pslist[i].c_str());
            --numlines;
          } else {
            outline("AND OTHERS ...");
            --numlines;
            break;
          }
        }

        outline("");
        outline("Number of cached assets: %u, Approx. memory used: %s",
                taskLists.num_assets_cached,
                readableMemorySize(taskLists.asset_cache_memory).c_str());
        outline("Number of cached asset versions: %u, Approx. memory used: %s",
                taskLists.num_assetversions_cached,
                readableMemorySize(taskLists.version_cache_memory).c_str());
        outline("Number of strings cached: %u", taskLists.str_store_size);

      }
      sleep(delay);
    };
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 0;
}
