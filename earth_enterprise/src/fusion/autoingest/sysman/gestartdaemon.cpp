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
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <notify.h>
#include <khFileUtils.h>
#include <khGetopt.h>
#include <geInstallPaths.h>
#include <khSpawn.h>
#include <geUsers.h>
#include <autoingest/.idl/Systemrc.h>

// ****************************************************************************
// ***  This is a separate program to take a service and make it into a daemon.
// ***
// ***  It does the typical fork, fork, followed by an exec of the final
// ***  target executable. This is a separate program because several of our
// ***  linux distributions have versions of fork and pthreads that cannot
// ***  coexist. Trying to create threads in an app that's been forked
// ***  results in the threads not working right.
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
          "\nusage: %s [options] <programname> <programarg>...\n"
          "   Supported options are:\n"
          "      --help | -?:    Display this usage message\n"
          "      --user:         User name to run as\n"
          "      --workingdir:   Directory to run in\n"
          "      --pidfile:      Where to write my pid\n"
          "      --logfile:      Where to write my log\n",
          prog);
  exit(1);
}


#define GetNextArg() ((argn < argc) ? argv[argn++] : 0)

int main(int argc, char *argv[]) {
  try {

    // get commandline options
    int argn;
    bool help = false;
    std::string user       = Systemrc::FusionUsername();
    std::string group      = Systemrc::UserGroupname();
    std::string workingdir = kGEOptPath;
    std::string pidfile;
    std::string logfile;
    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("user", user);
    options.opt("workingdir", workingdir);
    options.opt("pidfile", pidfile);
    options.opt("logfile", logfile);
    // needed so we don't pick up options that are part of the program
    // arguments
    options.stopAtFirstNonOption();
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
    if (logfile.empty()) {
      logfile = geLogFilename(programname);
    }


    // --- build a command line for the program we want to lanuch ---
    CmdLine cmdline;
    cmdline.push_back(programname);
    if (argn < argc) {
      std::copy(&argv[argn], &argv[argc], back_inserter(cmdline));
    }


    // --- Make sure another one of me is not already running ---
    // Note: This logic has a HUGE race condition. Two processes could check at
    // the same time and both proceed to become the "only one" running. Since
    // the frequency of starting a daemon is quite low, the chances of two
    // trying to start at the same time are very small. All the system level
    // routines for doing this exclusion ('startproc','start_daemon','daemon')
    // have the same race condition, so I don't feel too bad. :-)
    if (khExists(pidfile)) {
      FILE *pidFILE = fopen(pidfile.c_str(), "r");
      if (!pidFILE) {
        notify(NFY_FATAL, "Unable to open %s: %s",
               pidfile.c_str(), khstrerror(errno).c_str());
      }
      int pid;
      fscanf(pidFILE, "%d\n", &pid);
      fclose(pidFILE);

      // see if the process is still around
      if (khPidActive(pid)) {
        notify(NFY_FATAL, "Another %s is running with pid %d",
               programname, pid);
      } else {
        khUnlink(pidfile);
      }
    }

    // --- Optionally change our user ---
    if (user.size()) {
      geUserId userid(user, group);
      // We know we are running gestartdaemon as root.
      // We don't want the service enabled to be able to run as root.
      // So we reset its ruid and rgid rather than euid and egid
      userid.SwitchRealToThis();
    }

    // --- Change our working directory ---
    // This is where a core file for this process would go.
    if (chdir(workingdir.c_str()) == -1) {
      notify(NFY_FATAL, "Unable to chdir to %s: %s",
             workingdir.c_str(), khstrerror(errno).c_str());
    }

    // --- clear our umask ---
    // just in case the user that started us has an overly restrictive
    // umask (like root normally does)
    (void) umask(0002);


    // This logic for "going daemon" comes from W. Richard Stevens in his book,
    // "Unix Network Programming" Volume 1, Second Edition, section 12.4

    // --- Get us running in the background ---
    // Fork and have the parent exit. This also makes sure we're
    // not a process group leader (needed for call to setsid below).
    pid_t pid = fork();
    if (pid < 0) {
      notify(NFY_FATAL, "Unable to fork to become a daemon: %s",
             khstrerror(errno).c_str());
    } else if (pid > 0) {
      // parent
      exit(0);
    }

    // --- Make sure we have no controlling terminal ---
    // This makes a new session and process group, making this process the
    // leader for both. A side effect of this is that we no longer have a
    // controlling terminal
    (void) setsid();


    // --- Make sure we can't re-aquire a controlling terminal ---
    // Fork again and have the parent exit. We have to ignore SIGHUP first
    // or the new child will exit when the new group leader exits.
    (void) signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid < 0) {
      notify(NFY_FATAL, "Unable to fork to become a daemon: %s",
             khstrerror(errno).c_str());
    } else if (pid > 0) {
      // parent
      exit(0);
    }


    // --- Write the pidfile ---
    FILE *pidFILE = fopen(pidfile.c_str(), "w");
    if (!pidFILE) {
      notify(NFY_FATAL, "Unable to write %s: %s",
             pidfile.c_str(), khstrerror(errno).c_str());
    }
    fprintf(pidFILE, "%d\n", khPid());
    fclose(pidFILE);


    // --- get stdin from /dev/null ---
    {
      (void)::close(0);
      int fd = ::open("/dev/null", O_RDONLY);
      if (fd > 0) {
        // if we succeeded, but with a non-0 fd,
        // dup2 the fd over to 0
        (void)::dup2(fd, 0);
        (void)::close(fd);
      }
    }

    // --- put stdout to logfile ---
    {
      int fd = ::open(logfile.c_str(), O_WRONLY|O_CREAT|O_APPEND, 0666);
      if (fd == -1) {
        notify(NFY_FATAL, "Unable to open logfile %s: %s",
               logfile.c_str(), khstrerror(errno).c_str());
      } else {
        // if we succeeded dup2 the fd over to 1
        (void)::close(1);
        (void)::dup2(fd, 1);
        (void)::close(fd);
      }
    }

    // --- put stderr to logfile as well ---
    // save the old stderr (just in case we need to report errors
    // after the execvp)
    int altstderr = dup(2);
    // set my alt to auto close on exec, that way the daemon won't have extra
    // fd's laying around
    fcntl(altstderr, F_SETFD, FD_CLOEXEC);
    (void)::close(2);
    (void)::dup2(1, 2);

    // --- close all remaining file descriptors ---
    // If they're not open, the call to ::close won't hurt
    for (int fd = 3; fd < FD_SETSIZE; ++fd) {
      if (fd != altstderr) {
        (void)::close(fd);
      }
    }

    // --- launch program with the cmdline we built above ---
    cmdline.Execvp();
    int saveerrno = errno;


    // restore the fd under stderr
    (void)::close(2);
    (void)::dup2(altstderr, 2);

    notify(NFY_FATAL, "Unable to execvp %s: %s as (user:%s,group:%s)",
           cmdline[0].c_str(), khstrerror(saveerrno).c_str(),
	   user.c_str(), group.c_str());

    return 1;
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }
  return 1;
}
