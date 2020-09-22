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

#include "fusion/autoingest/sysman/khResourceProvider.h"

#include <sys/ptrace.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

#include "builddate.h"
#include "fusion/fusionversion.h"
#include "fusion/autoingest/Misc.h"
#include "fusion/autoingest/MiscConfig.h"
#include "fusion/autoingest/.idl/Systemrc.h"
#include "fusion/autoingest/.idl/storage/AssetDefs.h"
#include "fusion/autoingest/khVolumeManager.h"
#include "fusion/autoingest/khFusionURI.h"
#include "fusion/autoingest/sysman/khResourceManagerProxy.h"
#include "fusion/autoingest/geAssetRoot.h"
#include "fusion/config/gefConfigUtil.h"
#include "common/procpidstats.h"
#include "common/RuntimeOptions.h"
#include "common/khnet/SocketException.h"
#include "common/khFileUtils.h"
#include "common/khSpawn.h"
#include "common/khstl.h"
#include "common/performancelogger.h"

// options set using PTRACE_SETOPTIONS
#define PTRACE_O_TRACEEXIT      0x00000040
// Wait extended result codes for the above trace options.
#define PTRACE_EVENT_EXIT       6
#define PTRACE_SETOPTIONS (__ptrace_request)0x4200


khResourceProvider theResourceProvider;


// ****************************************************************************
// ***  FindJobBy* routines
// ****************************************************************************
khResourceProvider::JobIter
khResourceProvider::FindJobById(std::uint32_t jobid)
{
  return std::find_if(jobs.begin(), jobs.end(),
                      mem_var_pred_ref<std::equal_to>(&Job::jobid, jobid));
}

// ****************************************************************************
// ***  SignalLoop
// ****************************************************************************
void
khResourceProvider::SignalLoop(void)
{
  sigset_t waitset;
  sigemptyset(&waitset);
  sigaddset(&waitset, SIGINT);
  sigaddset(&waitset, SIGTERM);


  int sig;
  sigwait(&waitset, &sig);

  notify(NFY_NOTICE, "Caught signal %d", sig);

  // tell the others we want to exit & kill all running jobs
  {
    khLockGuard lock(mutex);
    wantexit = true;
  }



  // kill all running jobs
  // give the processes 2 tries (2 sec each) before killing w/ SIGKILL
  unsigned int numtries = 0;
  while (jobs.size()) {
    ++numtries;
    bool dokill = numtries > 2;
    notify(NFY_NOTICE, "Shutting down: sending %s to %lu jobs",
           dokill ? "SIGKILL" : "SIGTERM",
           static_cast<long unsigned>(jobs.size()));
    {
      khLockGuard lock(mutex);
      CleanJobsAndVolumes(dokill);

      if (jobs.size() == 0) {
        break;
      }
    }
    sleep(2);
  }
  notify(NFY_NOTICE, "Shutting down: 0 jobs left");
  notify(NFY_NOTICE, "Exiting");
  exit(0);
}

// ****************************************************************************
// ***  Run - main loop for primary thread
// ****************************************************************************
void
khResourceProvider::Run(void)
{
  // block the signals I'm going to wait for before any threads get started
  sigset_t waitset;
  sigemptyset(&waitset);
  sigaddset(&waitset, SIGINT);
  sigaddset(&waitset, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &waitset, 0);

  // Verify fusion version
  geAssetRoot::AssertVersion(AssetDefs::AssetRoot());

  Systemrc systemrc;
  LoadSystemrc(systemrc);

  if (getenv("KH_NFY_LEVEL") == NULL)
  {
       std::uint32_t logLevel = systemrc.logLevel;
       notify(NFY_NOTICE,"system log level: %s",
         khNotifyLevelToString(static_cast<khNotifyLevel>(logLevel)).c_str());
       setNotifyLevel(static_cast<khNotifyLevel>(logLevel));
  }
  std::uint32_t numCPUs = systemrc.maxjobs;
  PERF_CONF_LOGGING( "rprovider_config_numcpus", "numcpus", numCPUs  );
  notify(NFY_WARN, "khResourceProvider: systemrc.maxjobs =  %d",  numCPUs  );
  // start the SignalLoop thread - listens for SIGINT & SIGTERM
  RunInDetachedThread
    (khFunctor<void>(std::mem_fun(&khResourceProvider::SignalLoop), this));


  // start the CheckVolumeAvailLoop thread
  RunInDetachedThread
    (khFunctor<void>
     (std::mem_fun(&khResourceProvider::CheckVolumeAvailLoop),
      this));

  // start the PruneLoop thread
  khFunctor<void> prunefunc(std::mem_fun(&khResourceProvider::PruneLoop),
                            this);
  khThread pruner(prunefunc);
  pruner.run();

  while (1) {
    static const int firstWaitSecs = 3;
    static const int waitSecs = 15;
    FusionConnection::Handle manager;
    QString error;

    // connect to resource manager
    bool first = true;
    do {
      if (first) {
        sleep(firstWaitSecs);
      } else {
        sleep(waitSecs);
      }
      if (WantExit()) {
        return;
      }
      manager = FusionConnection::TryConnectTokhResourceManager(error);
      if (WantExit()) {
        return;
      }
      if (first && !manager) {
        notify(NFY_WARN, "Unable to connect to resource manager: %s",
               error.latin1());
        notify(NFY_WARN, "Will keep retrying every %d seconds",
               waitSecs);
      }
      first = false;
    } while (!manager);

    // send ProviderConnRequest

    PERF_CONF_LOGGING( "rprovider_register_resource_numcpu", khHostname(), numCPUs  );
    ProviderConnectMsg connreq(khHostname(), numCPUs, GEE_VERSION);
    if (!manager->TryNotify("ProviderConnectMsg", connreq, error)) {
      notify(NFY_WARN, "Unable to talk to resource manager: %s",
             error.latin1());
      // skip the rest of the loop and try to connect again
      sleep(waitSecs);
      continue;
    }

    {
      // clear any leftover msgs from before we connected to the
      // resource manager
      sendQueue->clear();
      dispatchQueue->clear();

      // start the SendLoop thread
      khFunctor<void> func(std::mem_fun(&khResourceProvider::SendLoop),
                           this,
                           manager);
      khThread sender(func);
      sender.run();

      // start the DispatchLoop thread
      khFunctor<void> func2(std::mem_fun(&khResourceProvider::DispatchLoop),
                            this);
      khThread dispatcher(func2);
      dispatcher.run();

      // Socket receive loop
      try {
        while (1) {
          FusionConnection::RecvPacket msg;
          manager->ReceiveMsg(msg);

          if (WantExit()) {
            break;
          }


          std::string cmdname = msg.cmdname();
          // because we can't write to this socket (the SendLoop
          // does), we only support the dispatch of NotifyMsg's
          if (msg.msgType == FusionConnection::NotifyMsg) {
            notify(NFY_DEBUG, "Handling notify %s", cmdname.c_str());
            dispatchQueue->push(msg);
          } else {
            notify(NFY_WARN, "Unsupported message type: %d",
                   msg.msgType);
            // we can't recover from this since the provider may be
            // waiting for a reply of some sort
            break;
          }
        }
      } catch (const SocketException &e) {
        if (e.errnum() == EPIPE) {
          notify(NFY_DEBUG, "Manager closed connection");
          // the manager has closed its side of the connection
          // No big deal, just exit the handler loop and
          // we'll try to connect again
        } else {
          notify(NFY_WARN, "Manager socket error: %s", e.what());
        }
      } catch (const std::exception &e) {
        notify(NFY_WARN, "Error handling manager messages: %s", e.what());
      } catch (...) {
        notify(NFY_WARN, "Unknown error handling manager messages");
      }

      // We're done reading
      // send a dummy command to the SendLoop thread to make
      // sure he exits
      sendQueue->push(SendCmd());

      // send a dummy command to the DispatchLoop thread to make
      // sure he exits. Go ahead and push it on the front. Anything
      // that's waiting to be started/stopped doesn't matter anyway.
      // We're going away and shutting everything down. No more messages
      // are going to be sent to the resource manager.
      dispatchQueue->push_front(FusionConnection::RecvPacket());

      // leaving scope will join with SendLoop & DispatchLoop threads
    }

    // to get to this point we must have lost connection
    // with the resource manager
    {
      khLockGuard lock(mutex);
      if (!wantexit) {
        CleanJobsAndVolumes(false);
      }
    }

    // leaving scope will close the manager connection handle
  }

  // leaving scope will join with PruneLoop
}


// ****************************************************************************
// ***  SendLoop
// ****************************************************************************
void
khResourceProvider::SendLoop(FusionConnection::Handle manager)
{
  khResourceManagerProxy proxy(manager);

  while (1) {
    try {
      SendCmd cmd = sendQueue->pop();
      if (cmd.IsNoOp()) {
        // Signal that I'm supposed to go away
        break;
      }
      try {
        cmd(&proxy);
      } catch (...) {
        // can't send, manager connection must be lost
        break;
      }
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Caught exception popping send queue: %s",
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Caught unknown popping send queue");
    }
  }

  // clean out any left overs
  sendQueue->clear();
}

// ****************************************************************************
// ***  DispatchLoop
// ****************************************************************************
void
khResourceProvider::DispatchLoop(void)
{
  while (1) {
    try {
      FusionConnection::RecvPacket msg(dispatchQueue->pop());
      if (msg.msgType != FusionConnection::NotifyMsg) {
        // Signal that I'm supposed to go away
        break;
      }
      try {
        DispatchNotify(msg);
      } catch (const std::exception &e) {
        notify(NFY_WARN, "Error: %s: %s",
               msg.cmdname().c_str(), e.what());
      } catch (...) {
        notify(NFY_WARN, "Error: %s: Unknown error",
               msg.cmdname().c_str());
      }
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Caught exception popping dispatch queue: %s",
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Caught unknown popping dispatch queue");
    }
  }

  // clean out any left overs
  dispatchQueue->clear();
}

// ****************************************************************************
// ***  PruneLoop
// ****************************************************************************
void
khResourceProvider::PruneLoop(void)
{
  while (1) {
    try {
      std::string path(pruneQueue->pop());
      if (path.empty()) {
        // Signal that I'm supposed to go away
        break;
      }
      // If this fails it will already have emitted a warning.
      // There's nothing we can do about the failure so ignore the
      // return code.
      (void)khPruneFileOrDir(path);
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Caught exception popping prune queue: %s",
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Caught unknown popping prune queue");
    }
  }
}


// ****************************************************************************
// ***  ProgressLoop
// ****************************************************************************
void
khResourceProvider::SendProgress(std::uint32_t jobid, double progress,
                                 time_t progressTime) {
  khLockGuard lock(mutex);
  JobIter job = FindJobById(jobid);
  if (Valid(job)) {
    // notify the resource manager
    sendQueue->push
      (SendCmd(std::mem_fun(&khResourceManagerProxy::JobProgress),
               JobProgressMsg(jobid,
                              job->beginTime,
                              progressTime,
                              progress),
               0 /* timeout */));
  }
}


#if 0
void
khResourceProvider::ReadProgress(int readfd, std::uint32_t jobid)
{
  double progress = 0.0;

  // notify the resource manager the first time
  SendProgress(jobid, progress, time(0));

  // now read the rest of them
  while (khReadAll(readfd, &progress, sizeof(progress))) {
    SendProgress(jobid, progress, time(0));
  }
  (void)::close(readfd);
}
#endif


// ****************************************************************************
// ***  CheckVolumeAvailLoop
// ****************************************************************************
void
khResourceProvider::CheckVolumeAvailLoop(void)
{
  while (1) {
    // TODO: - finalize this number 2 - 5 minutes?
    sleep(10);

    VolResMap tocheck;
    {
      // lock the mutex and make a copy of what we want to check
      khLockGuard lock(mutex);
      if (wantexit)
        return;
      tocheck = volResMap;
    }

    // process each volume and determine it's available space
    for (VolResMap::const_iterator c = tocheck.begin();
         c != tocheck.end(); ++c) {
      std::string volname = c->first;
      const VolumeReservations &volres = c->second;
      const VolumeDefList::VolumeDef* voldef =
        theVolumeManager.GetVolumeDef(volname);
      std::uint64_t avail = 0;

      if (voldef) {
        // calculate filesystem free space (minus reserve)
        (void)khGetFilesystemFreeSpace(voldef->localpath, avail);
        avail -= std::min(avail, voldef->reserveSpace);

        // subtract off the reserved portions
        for (std::vector<VolumeReservations::Item>::const_iterator res =
               volres.items.begin();
             res != volres.items.end(); ++res) {
          std::uint64_t ressize = res->size;

          // don't bother to calculate the used size if the
          // ressize wasn't set
          if (ressize) {
#if 0
            std::string respath =
              voldef->localpath + "/" + res->path;
            std::uint64_t used = khGetUsedSpace(respath);
            ressize -= std::min(ressize, used);
#endif
          }

          avail -= std::min(avail, ressize);
        }
      }

      sendQueue->push
        (SendCmd(std::mem_fun(&khResourceManagerProxy::SetVolumeAvail),
                 VolumeAvailMsg(volname, volres.serial, avail),
                 0 /* timeout */));
    }
  }
}


// ****************************************************************************
// ***  StartJob
// ****************************************************************************
void
khResourceProvider::StartJob(const StartJobMsg &start)
{
  // the mutex must be locked
  assert(!mutex.trylock());

  // add the new job to my list
  jobs.push_back(Job(start.jobid));

  // start the job thread
  unsigned int cmdTries = std::max(MiscConfig::Instance().TriesPerCommand, uint(1)); // Have to try at least once
  unsigned int sleepBetweenTriesSec = MiscConfig::Instance().SleepBetweenCommandTriesSec;
  jobThreads->run
    (khFunctor<void>(std::mem_fun(&khResourceProvider::JobLoop),
                     this, start, cmdTries, sleepBetweenTriesSec));
}


bool
khResourceProvider::ExecCmdline(JobIter job,
                                const std::vector<std::string> &cmdline)
{
  // prepend command to logfile & flush it to disk
  if (job->logfile) {
    fprintf(job->logfile, "\nCOMMAND:");
    for (std::vector<std::string>::const_iterator arg =
           cmdline.begin();
         arg != cmdline.end(); ++arg) {
      fputc(' ', job->logfile);
      fputs(arg->c_str(), job->logfile);
    }
    fprintf(job->logfile,
            "\n---------- Start Command Output ----------\n");
    fflush(job->logfile);
  }
  int logfd = job->logfile ? fileno(job->logfile) : -1;


  pid_t pid = ::fork();
  if (pid < 0) {
    // *** unable to fork ***

    // finish & close logfile
    if (job->logfile) {
      fprintf(job->logfile, "FATAL ERROR: Unable to fork\n");
    }
    return false;
  } else if (pid == 0) {
    // *** inside child process ***

    // clear my signal mask so the child process will
    // pick up SIGINT & SIGTERM
    sigset_t waitset;
    sigemptyset(&waitset);
    pthread_sigmask(SIG_SETMASK, &waitset, 0);

    // Make myself a progress group leader. This way my parent can
    // kill the group (same # as my pid) and have all my children go
    // away too. This is critical if we're about to exec a shell
    // script. Since sending SIGTERM to the shell will kill the
    // shell, but leave it's children running.  This shouldn't
    // fail. But even if it does there is nothing we can do about
    // it.
    (void) setsid();


    // clear any debug env variables
    unsetenv("KH_NFY_LEVEL");
    unsetenv("KH_ENABLE_ALL");

    // close all fds except write half of pipe & logfile
    for (int fd = 0; fd < FD_SETSIZE; ++fd) {
      if (fd != logfd) {
        (void)::close(fd);
      }
    }

    // move logfd to fileno 1 & 2 (stdout, stderr)
    if (logfd >= 0) {
      if (logfd != 1) {
        if (::dup2(logfd, 1) < 0) {
          (void)::close(logfd);
          logfd = -1;
        } else {
          (void)::close(logfd);
          logfd = 1;
        }
      }
      if (logfd == 1) {
        (void)::dup2(1, 2);
      }
      if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        notify(NFY_WARN,
               "Cannot enable PTRACE_TRACEME to collect process status\n");
      }
    }

    // get stdin from /dev/null
    {
      int fd = ::open("/dev/null", O_RDONLY);
      if (fd > 0) {
        // if we succeeded, but with a non-0 fd,
        // dup2 the fd over to 0
        (void)::dup2(fd, 0);
        (void)::close(fd);
      }
    }

    // format the cmdline and call execvp
    const char *argv[cmdline.size() + 1];
    transform(cmdline.begin(), cmdline.end(), argv,
              std::mem_fun_ref(&std::string::c_str));
    argv[cmdline.size()] = 0;
    execvp(argv[0], const_cast<char* const*>(argv));

    // error handling if we can't execvp
    // use low level write since we've clobbered all the FILE* stuff
    // by closing & duping the fds
    std::string error = "FATAL ERROR: Unable to exec\n";
    (void)::write(1, error.c_str(), error.size());
    _exit(1);
  } else if (pid > 0) {
    // *** inside parent process - nothing to do

    job->pid = pid;
  }
  return true;
}

void
khResourceProvider::JobLoop(StartJobMsg start, const unsigned int cmdTries, const unsigned int sleepBetweenTriesSec)
{
  const std::uint32_t jobid = start.jobid;
  time_t endtime = 0;
  bool success = false;
  bool logTotalTime = false;
  bool progressSent = false;

  khLockGuard lock(mutex);
  JobIter job = FindJobById(jobid);
  if (!Valid(job)) {
    // somebody already asked for me to go away
    return;
  }

  for (unsigned int cmdnum = 0; cmdnum < start.commands.size(); ++cmdnum) {
    // Write out the overall time if we run more than one command
    logTotalTime = (cmdnum > 0);

    time_t cmdtime = time(0);
    if (!job->beginTime) {
      job->beginTime = cmdtime;
    }

    if (!job->logfile) {
      StartLogFile(job, start.logfile);
    }

    success = false;
    for (unsigned int tries = 0; tries < cmdTries && !success; ++tries) {
      if (tries > 0) {
        // Write out the overall time if we run a command more than once.
        logTotalTime = true;
        if (job->logfile) {
          LogRetry(job, tries, cmdTries, sleepBetweenTriesSec);
        }
        if (sleepBetweenTriesSec > 0) {
          {
            // Release the lock while we sleep
            khUnlockGuard unlock(mutex);
            sleep(sleepBetweenTriesSec);
          }
          // Once we have the lock again, check if someone deleted the job while
          // we were sleeping
          job = FindJobById(jobid);
          if (!Valid(job)) return;
        }
      }
      success = RunCmd(job, jobid, start.commands[cmdnum], cmdtime, endtime, progressSent);
      if (!Valid(job)) return;  // check if somebody already asked for me to go away
    }
    // If we failed on all of the tries, give up
    if (!success) break;
  } /* for cmdnum */

  if (job->logfile && logTotalTime) {
    LogTotalTime(job, endtime - job->beginTime);
  }

  DeleteJob(job, success, job->beginTime, endtime);
}

bool
khResourceProvider::RunCmd(
    JobIter & job,
    std::uint32_t jobid,
    const std::vector<std::string> & command,
    time_t cmdtime,
    time_t & endtime,
    bool & progressSent) {

  // ***** Launch the command *****
  if (!ExecCmdline(job, command)) {
    return false;
  }

  bool success = false;
  pid_t waitfor = job->pid;
  bool coredump = false;
  int signum = -1;
  std::string status_string;
  {
    // release the lock while we wait for the process to finish
    khUnlockGuard unlock(mutex);

    // notify the resource manager the first time
    if (!progressSent) {
      SendProgress(jobid, 0, time(0));
      progressSent = true;
    }

    // ***** wait for command to finish *****
    if (job->logfile) {
      // Collect process status summary just before it exits.
      GetProcessStatus(waitfor, &status_string, &success, &coredump, &signum);
    } else {
      WaitForPid(waitfor, success, coredump, signum);
    }

    // get the endtime before re-acquiring the lock
    endtime = time(0);
  }

  // now that we have the lock again, make sure the job hasn't been
  // deleted while we were waiting for it to finish
  job = FindJobById(jobid);
  if (!Valid(job)) return false;

  job->pid = 0;

  // ***** report command status *****
  if (job->logfile) {
    LogCmdResults(job, status_string, signum, coredump, success, cmdtime, endtime);
  }
  return success;
}

void
khResourceProvider::StartLogFile(JobIter job, const std::string &logfile) {
  job->logfile = fopen(logfile.c_str(), "w");
  if (job->logfile) {
    // open the logfile & write the header
    fprintf(job->logfile, "BUILD HOST: %s\n",
            khHostname().c_str());
    fprintf(job->logfile, "FUSION VERSION %s, BUILD %s\n",
            GEE_VERSION, BUILD_DATE);
    QString runtimeDesc = RuntimeOptions::DescString();
    if (!runtimeDesc.isEmpty()) {
      fprintf(job->logfile, "OPTIONS: %s\n", runtimeDesc.latin1());
    }
    fprintf(job->logfile, "STARTTIME: %s\n",
            GetFormattedTimeString(job->beginTime).c_str());
  }
}

void
khResourceProvider::GetProcessStatus(pid_t pid, std::string* status_string,
                                     bool* success, bool* coredump, int* signum) {
  ProcPidStats::GetProcessStatus(pid, status_string, success, coredump, signum);
}

void
khResourceProvider::WaitForPid(pid_t waitfor, bool &success, bool &coredump,
                               int &signum) {
  // I don't care about the return value, the pass by ref params will
  // be set correctly either way
  (void)khWaitForPid(waitfor, success, coredump, signum, NULL);
}

void
khResourceProvider::LogCmdResults(
    JobIter job,
    const std::string &status_string,
    int signum,
    bool coredump,
    bool success,
    time_t cmdtime,
    time_t endtime) {
  fflush(job->logfile);
  // If process status information has been collected print that to log.
  if (!status_string.empty()) {
    fprintf(job->logfile, "%s", status_string.c_str());
  }
  fprintf(job->logfile,
          "---------- End Command Output ----------\n");
  if (signum != -1) {
    fprintf(job->logfile,
            "Process terminated by signal %d%s\n",
            signum,
            coredump ? " (core dumped)" : "");
  }

  fprintf(job->logfile, "ENDTIME: %s\n",
          GetFormattedTimeString(endtime).c_str());
  std::uint32_t elapsed = endtime - cmdtime;
  fprintf(job->logfile, "ELAPSEDTIME: %s\n",
          GetFormattedElapsedTimeString(elapsed).c_str());
  if (success) {
    fprintf(job->logfile, "COMPLETED SUCCESSFULLY\n");
  } else if ((signum == 2) || (signum == 15)) {
    fprintf(job->logfile, "CANCELED\n");
  } else {
    fprintf(job->logfile, "FAILED\n");
  }
}

void
khResourceProvider::LogRetry(JobIter job, unsigned int tries, unsigned int totalTries, unsigned int sleepBetweenTries) {
  fprintf(job->logfile, "\nRETRYING FAILED COMMAND after %d seconds, try %d of %d\n",
          sleepBetweenTries, tries + 1, totalTries);
  fflush(job->logfile);
}

void
khResourceProvider::LogTotalTime(JobIter job, std::uint32_t elapsed) {
  fprintf(job->logfile, "\nTOTAL ELAPSEDTIME: %s\n",
          GetFormattedElapsedTimeString(elapsed).c_str());
}

// ****************************************************************************
// ***  StopJob
// ****************************************************************************
void
khResourceProvider::StopJob(const StopJobMsg &stop)
{
  // the mutex must be locked
  assert(!mutex.trylock());

  JobIter job = FindJobById(stop.jobid);
  if (Valid(job)) {
    if (job->pid > 0) {
      // It's already running, so try to kill it. Killing -pid instead
      // of pid says to send the signal to all processes in the
      // process group pid. When we fork our chid we make it a process
      // group leader.
      notify(NFY_DEBUG, "Killing pgid %d", -job->pid);
      if (!khKillPid(-job->pid)) {
        // warning has already been emitted
        DeleteJob(job);
      }
    } else {
      // it's not running yet, taking it out of the list
      // will keep it from ever running
      DeleteJob(job);
    }
  }
}


// ****************************************************************************
// ***  CleanupVolume
// ****************************************************************************
void
khResourceProvider::CleanupVolume(const VolumeReservations &volres)
{
  // the mutex must be locked
  assert(!mutex.trylock());

  for (std::vector<VolumeReservations::Item>::const_iterator i =
         volres.items.begin();
       i != volres.items.end(); ++i) {
    CleanPath(khFusionURI(volres.name, i->path).LocalPath());
  }
}

namespace {
bool
khRenameToUnique(const std::string &oldname, std::string &retname)
{
  static unsigned int counter = 0;
  char buf[100];
  snprintf(buf, sizeof(buf), ".%d.%d.%u",
           static_cast<int>(time(0)), static_cast<int>(khPid()), ++counter);
  buf[sizeof(buf)-1] = 0;
  retname = oldname + buf;
  return khRename(oldname, retname);
}
}  // namespace

void
khResourceProvider::CleanPath(const std::string &path)
{
  // get list of all files to delete (incl. piggybacks & overflows)
  std::vector<std::string> todelete;
  todelete.push_back(path);
  khGetOverflowFilenames(path, todelete);

  for (std::vector<std::string>::const_iterator d = todelete.begin();
       d != todelete.end(); ++d) {
    if (khExists(*d)) {
      std::string newname;
      if (khRenameToUnique(*d, newname)) {
        pruneQueue->push(newname);
      } else {
        pruneQueue->push(*d);
      }
    }
  }
}


// ****************************************************************************
// ***  ChangeVolumeReservations
// ****************************************************************************
void
khResourceProvider::ChangeVolumeReservations(const VolumeReservations &res)
{
  // the mutex must be locked
  assert(!mutex.trylock());

  volResMap[res.name] = res;
}

void
khResourceProvider::DeleteJob(JobIter which,
                              bool success,
                              time_t beginTime, time_t endTime)
{
  // tell the resource manager I'm done so he can release
  // the resources
  // Don't report that the tasks are done if we're trying to go away.
  // We'll have signaled the job and it will be failed. What we want to
  // happen is for the resource manager to lose contact with us and
  // reschedule our tasks.
  if (!wantexit) {
    sendQueue->push(SendCmd
                    (std::mem_fun(&khResourceManagerProxy::JobDone),
                     JobDoneMsg(which->jobid,
                                success,
                                beginTime,
                                endTime),
                     0 /* timeout */));
  }

  if (which->logfile) {
    fclose(which->logfile);
    which->logfile = 0;
  }

  // remove job from list
  jobs.erase(which);

  // if we wan't to go away and this was the last job to report back,
  // go ahead and exit(). We have to exit since the read thread will
  // block forever waiting for the asset manager to say something
  if (wantexit) {
    notify(NFY_NOTICE, "Shutting down: %lu jobs left",
           static_cast<long unsigned>(jobs.size()));
    if (jobs.empty()) {
      notify(NFY_NOTICE, "Exiting");
      exit(0);
    }
  }
}


// ****************************************************************************
// ***  CleanJobsAndVolumes
// ****************************************************************************
void
khResourceProvider::CleanJobsAndVolumes(bool sigkill)
{
  // the mutex must be locked
  assert(!mutex.trylock());

  unsigned int i = 0;
  while (i < jobs.size()) {
    Job *j = &jobs[i];
    if (j->pid > 0) {
      // It's already running, so try to kill it. Killing -pid instead
      // of pid says to send the signal to all processes in the
      // process group pid. When we fork our chid we make it a process
      // group leader.
      notify(NFY_DEBUG, "Killing pid %d", -j->pid);
      if (khKillPid(-j->pid, sigkill)) {
        ++i;
      } else {
        // warning has already been emitted
        DeleteJob(jobs.begin()+i);
        // don't increment i
      }
    } else {
      // it's not running yet, taking it out of the list
      // will keep it from ever running
      DeleteJob(jobs.begin()+i);
      // don't increment i
    }
  }

  // clear reservation list
  // don't try to clean the files yet, the jobs may be writing new files
  // into these dirs anyway.
  // the Reservations in the resource manager will do the cleanup
  // if the resource manager crashed then when we connect the next time
  // I'll get a CleanupVolume message for each volumne that I manage
  volResMap.clear();
}


// ****************************************************************************
// ***  khResourceProvider
// ****************************************************************************
khResourceProvider::khResourceProvider(void)
    : sendQueue(new SendQueue()),
      dispatchQueue(new DispatchQueue()),
      pruneQueue(new PruneQueue()),
      jobThreads(khThreadPool::Create(0, 2)),
      wantexit(false)
{
}


// ****************************************************************************
// ***  ~khResourceProvider
// ****************************************************************************
khResourceProvider::~khResourceProvider(void)
{
  // don't delete progress threads - it's currently not allowed

  // don't delete sendQueue, dispatchQueue or pruneQueue
  // If we're getting destroyed from the signal loop, there may still be
  // threads blocking on the various queues. To delete tem like that would
  // cause the app to assert on exit.

  // Since there's only ever one khResourceProvider and it only goes away at
  // the end of the program, leaving these allocated will cause no harm.
}
