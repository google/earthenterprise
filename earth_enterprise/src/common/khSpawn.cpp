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


#include "khSpawn.h"
#include <functional>
#include <algorithm>
// for FD_SETSIZE
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <notify.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>
#include <errno.h>
#include <khstl.h>
#include <geInstallPaths.h>
#include <khFileUtils.h>
#include <khException.h>
#include <khstrconv.h>
#include <geUsers.h>

// ****************************************************************************
// ***  Cmdline
// ****************************************************************************
std::string CmdLine::AsString(void) const {
  return join(begin(), end(), std::string(" "));
}


void CmdLine::Execvp(void) const {
  // reformat the STL vector into the lower level format
  const char *fargv[size()+1];
  std::transform(begin(), end(), fargv,
                 std::mem_fun_ref(&std::string::c_str));
  fargv[size()] = 0;

  // forward the call
  ::execvp(fargv[0], const_cast<char* const*>(fargv)); // should not return
}


pid_t CmdLine::Spawn(SpawnUser spawn_user) const {
  pid_t pid = fork();
  if (pid == -1) {
    // - fork error - likely out of memory or processes
    notify(NFY_WARN, "Unable to fork");
  } else if (pid == 0) {
    // - child -

    if (spawn_user == SpawnAsRealUser) {
      geUserId::RestoreEffectiveUserToRealUser();
    }

    Execvp();  // shouldn't return
    notify(NFY_WARN, "Unable to exec %s", AsString().c_str());
    _exit(1);
  } else {
    // - parent -
    // nothing to do
  }
  return pid;
}

bool CmdLine::System(SpawnUser spawn_user) const {
  pid_t pid = Spawn(spawn_user);
  if (pid > 0) {
    int ret;
    int status;
    do {
      ret = waitpid(pid, &status, 0);
    } while ((ret == -1) && (errno == EINTR));
    return ((ret == pid) &&
            WIFEXITED(status) &&
            (WEXITSTATUS(status) == 0));
  }
  return false;
}



bool
khWaitForPid(pid_t waitfor, bool &success, bool &coredump, int &signum,
             struct rusage *rusage)
{

  success = false;
  coredump = false;
  signum = -1;

  int status;
  pid_t pid;

  if (rusage == NULL) {  // In this case wait for this process
    do {
      pid = waitpid(waitfor, &status, 0);
    } while ((pid == -1) && (errno == EINTR));
  } else {  // In this case wait for this process and its children and collect
            // their cumulative usage information in rusage. The -pid syntax is
            // to wait for all pids with process group id of pid. Note: while
            // calling we have set the pgid to forked process using setsid for
            // this convinience.
    do {
      pid = wait4(-waitfor, &status, 0, rusage);
    } while ((pid != waitfor && pid > 0) || (pid == -1 && errno == EINTR));
  }

  if (pid != waitfor) {
    // can't waitpid for some reason
    return false;
  } else {
    // interpret wait status
    if (WIFEXITED(status)) {
      success = (WEXITSTATUS(status) == 0);
    } else if (WIFSIGNALED(status)) {
      signum = WTERMSIG(status);
#ifdef WCOREDUMP
      if (WCOREDUMP(status)) {
        coredump = true;
      }
#endif
    }
    return true;
  }
}


std::string gePidFilename(const std::string &procname)
{
  return khComposePath(kGERunPath, procname + ".pid");
}

std::string geLogFilename(const std::string &procname)
{
  return khComposePath(kGELogPath, procname + ".log");
}

// should be run as root so we can call khPidActive
bool geCheckPidFile(const std::string &procname,
                    const std::string &pid_file)
{
  std::string pidfile = pid_file.empty() ? gePidFilename(procname) : pid_file;

  if (khExists(pidfile)) {
    std::string pidstr;
    if (khReadSimpleFile(pidfile, pidstr)) {
      int pid = 0;
      FromString(pidstr, pid);
      if (pid > 0) {
        return khPidActive(pid);
      } else {
        throw khException(kh::tr("Unable to parse pid from %1").arg(pidfile.c_str()));
      }
    } else {
      throw khException(kh::tr("Unable to read pidfile %1").arg(pidfile.c_str()));
    }
  }
  return false;
}

bool
khPidActive(pid_t pid)
{
  // signum 0 -> don't actually send signal just see if you can
  return (::kill(pid, 0) == 0);
}

std::string
khHostname(void)
{
  char buf[256];
  gethostname(buf, sizeof(buf));
  buf[sizeof(buf)-1] = 0;
  return buf;
}

pid_t
khPid(void)
{
  return getpid();
}

bool
khKillPid(pid_t pid, bool sigkill)
{
  int ret = ::kill(pid, sigkill ? SIGKILL : SIGTERM);
  if (ret != 0) {
    notify(NFY_WARN, "Unable to send %s to pid %d: %s",
           sigkill ? "SIGKILL" : "SIGTERM",
           pid,
           khstrerror(errno).c_str());
    return false;
  } else {
    return true;
  }
}
