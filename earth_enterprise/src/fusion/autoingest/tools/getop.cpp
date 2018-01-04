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


#include <notify.h>
#include <unistd.h>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/khAssetManagerProxy.h>
#include <khSpawn.h>
#include <khGetopt.h>
#include <fusionversion.h>
#include <config/gefConfigUtil.h>
#include <autoingest/.idl/Systemrc.h>

// global for convenience
int numcols = 80;

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
          "      --delay <seconds>: Seconds to wait between refreshes\n",
          progn.c_str());
  exit(1);
}

int
main(int argc, char *argv[])
{
  try {
    std::string progname = argv[0];
    int argn;
    bool help  = false;
    int delay = 5;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("delay", delay);
    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);
    if (delay <= 0)
      usage(progname, "--delay must be positive");


    std::string master    = AssetDefs::MasterHostName();
    std::string assetroot = AssetDefs::AssetRoot();
    CmdLine clearscreen;
    clearscreen << "clear";

    while (1) {
      QString error;
      TaskLists taskLists;
      if (!khAssetManagerProxy::GetCurrTasks("dummy", taskLists,
                                             error)) {
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
          if (numlines > 5) {
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
        numlines -= 5;

        for (uint i = 0; i < pslist.size(); ++i) {
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
        outline("Number of cached assets: %u", taskLists.num_assets_cached);
        outline("Number of cached asset versions: %u",
                taskLists.num_assetversions_cached);

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
