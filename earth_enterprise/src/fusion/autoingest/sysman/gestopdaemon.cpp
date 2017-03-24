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


#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <notify.h>
#include <khFileUtils.h>
#include <khGetopt.h>
#include <khSpawn.h>

// define needed on redhat-7.3
#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED
#endif
#include <unistd.h>


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
          "\nusage: %s [options] <programname>\n"
          "   Supported options are:\n"
          "      --help | -?:    Display this usage message\n"
          "      --pidfile:      Where to read my pid\n",
          prog);
  exit(1);
}


#define GetNextArg() ((argn < argc) ? argv[argn++] : 0)

int main(int argc, char *argv[]) {

  // get commandline options
  int argn;
  bool help = false;
  std::string pidfile;
  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.opt("pidfile", pidfile);
  if (!options.processAll(argc, argv, argn)) {
    usage(argv[0]);
  }
  if (help) {
    usage(argv[0]);
  }

  // --- cmdline validation ---
  const char *programname = GetNextArg();
  if (!programname) {
    usage(argv[0], "<programname> not specified");
  }
  if (pidfile.empty()) {
    pidfile = gePidFilename(programname);
  }

  if (khExists(pidfile)) {
    FILE *pidFILE = fopen(pidfile.c_str(), "r");
    if (pidFILE) {
      int pid;
      fscanf(pidFILE, "%d\n", &pid);
      fclose(pidFILE);

      pid_t pgid = getpgid(pid);
      if ((pgid > 0) && (killpg(pgid, SIGTERM) == 0)) {
        khUnlink(pidfile);
        return 0;
      }
    }
    khUnlink(pidfile);
  }

  return 1;
}
