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


#include "khSystemManager.h"
#include "khAssetManager.h"
#include "khResourceManager.h"
#include "TaskRequirements.h"
#include <fusionversion.h>
#include <khThread.h>
#include <khFunctor.h>
#include <signal.h>
#include <geLockfileGuard.h>
#include <khSpawn.h>
#include <autoingest/geAssetRoot.h>
#include "common/performancelogger.h"
#include "fusion/config/gefConfigUtil.h"



// ****************************************************************************
// ***  khSystemmanager
// ****************************************************************************
khSystemManager theSystemManager;

khSystemManager::khSystemManager(void) : wantexit(false)
{
}

khSystemManager::~khSystemManager(void)
{
}

void
khSystemManager::SetWantExit(void)
{
  khLockGuard guard(mutex);

  // only process the SetWantExit the first time
  if (!wantexit) {
    wantexit = true;

    // send an empty change list to make sure the AssetNotifierLoop
    // wakes up and notices that we "wantExit"
    theAssetManager.changesQueue.push(AssetChanges());

    // send a NoOp cmd to make sure AssetCmdLoop wakes up
    // and notices that we "wantExit"
    theAssetManager.assetCmdQueue.push(khAssetManager::AssetCmd());

    // send a NoOp cmd to make sure TaskCmdLoop wakes up
    // and notices that we "wantExit"
    theResourceManager.taskCmdQueue.push(khResourceManager::TaskCmd());

    // wake up the ActivateLoop
    {
      // this lock is safe since nobody that calls SetWantExit will
      // ever be holding one of the mutex's
      khLockGuard lock(theResourceManager.mutex);
      theResourceManager.activateCondVar.signal_one();
    }
  }
}

// when SIGHUP is given, reload systemrc
void systemrc_reload(int signum)
{
  notify(NFY_WARN, "Received SIGHUP, Reloading systemrc...");
  Systemrc systemrc;
  LoadSystemrc(systemrc,true);
  std::uint32_t logLevel = systemrc.logLevel;
  notify(NFY_WARN, "system log level changed to: %s",
          khNotifyLevelToString(static_cast<khNotifyLevel>(logLevel)).c_str());
  setNotifyLevel(static_cast<khNotifyLevel>(logLevel));
}

void handleExitSignals(int signum)
{
  notify(NFY_WARN, "Received SIGINT or SIGTERM, Exiting...");
  theSystemManager.SetWantExit();
}


void
khSystemManager::SignalLoop(void)
{
  // This is not a loop.  We're not iterating over any messages.
  // We just install signal handlers, and return.
  signal(SIGINT, handleExitSignals);
  signal(SIGTERM, handleExitSignals);
  signal(SIGHUP, systemrc_reload);
}


void
khSystemManager::Run(void)
{
  // block the signals I'm going to wait for before any threads get started
  sigset_t waitset;
  sigemptyset(&waitset);
  sigaddset(&waitset, SIGINT);
  sigaddset(&waitset, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &waitset, 0);

  // Verify fusion version
  geAssetRoot::AssertVersion(AssetDefs::AssetRoot());


  // Will write the hostname to the Active and remove it
  // on the way out. If we go down hard, the file will still be here,
  // but we'll just overwrte it.
  std::string active_fname =
    geAssetRoot::Filename(AssetDefs::AssetRoot(), geAssetRoot::ActiveFile);
  geLockfileGuard lock_file_guard(active_fname, khHostname());


  // reload my state that we save the last time we ran
  // do this before any threads get started
  theVolumeManager.Init();
  TaskRequirements::Init();
  theAssetManager.Init();
  theResourceManager.Init();

  {
    // use 'func' temporary so compiler doesn't get confused and think
    // I'm trying to declare a bunch of functions returning khThread

    // *** AssetNotifierLoop ***
    khFunctor<void> func = khFunctor<void>
                           (std::mem_fun(&khAssetManager::AssetNotifierLoop),
                            &theAssetManager);
    khThread assetNotifier(func);
    assetNotifier.run();


    // *** AssetCmdLoop ***
    func = khFunctor<void>(std::mem_fun(&khAssetManager::AssetCmdLoop),
                           &theAssetManager);
    khThread assetCmdLoop(func);
    assetCmdLoop.run();

    // *** TaskCmdLoop ***
    func = khFunctor<void>(std::mem_fun(&khResourceManager::TaskCmdLoop),
                           &theResourceManager);
    khThread taskCmdLoop(func);
    taskCmdLoop.run();


    // *** ActivateLoop ***
    func = khFunctor<void>(std::mem_fun(&khResourceManager::ActivateLoop),
                           &theResourceManager);
    khThread activateLoop(func);
    activateLoop.run();


    // *** ClientListenerLoop (detached) ***
    RunInDetachedThread
      (khFunctor<void>(std::mem_fun
                       (&khAssetManager::ClientListenerLoop),
                       &theAssetManager));


    // *** ProcessorListenerLoop (detached) ***
    RunInDetachedThread
      (khFunctor<void>(std::mem_fun
                       (&khResourceManager::ProviderListenerLoop),
                       &theResourceManager));


    // *** SignalLoop (detached) ***
    RunInDetachedThread
      (khFunctor<void>(std::mem_fun(&khSystemManager::SignalLoop),
                       this));

    // manually join with one of the normal threads, just to make sure
    // we don't start deleting the objects before the threads gets started
    // khThread needs to be fixed to not have this problem
    assetNotifier.join();

    // leaving scope will join with all the normal thread threads
    // (non detached) started above
  }

  // By the time we get here, somebody has already called SetWantExit()


  // We manually lock these here and NEVER release them (even before
  // dropping off the end of main). This ensures that the detached reader
  // and listener threads will NEVER be able to submit another job.
  theAssetManager.mutex.lock();
  theResourceManager.mutex.lock();

  // We're effectively down to one processing thread again - namely
  // the one that's running this routine
  //  - The cmdloop threads and the assetnotifier thread have been joined
  //  - The reader threads will block on the asset manager mutex or the
  //    task manager mutex that we locked above
  //  - The server socket listener threads may end up spawning one more
  // reader thread each, but that will block with all the other reader
  // threads
  //  - The signal thread may or may not still be out there running around,
  // but he doesn't do anything except set some flags


  // No more requests are accepted from clients
  // No more notifications are accepted from the processors
  // No new clients are accepted
  // No new processors are accepted
  // No more tasks are activated


  // Manually drain the processing queues
  bool didsomething;
  do {
    didsomething = false;

    {
      khAssetManager::AssetCmd cmd;
      while (theAssetManager.assetCmdQueue.trypop(cmd)) {
        didsomething = true;
        khAssetManager::PendingAssetGuard guard(theAssetManager);
        theAssetManager.ProcessAssetCmd(cmd);
      }
    }

    {
      khResourceManager::TaskCmd cmd;
      while (theResourceManager.taskCmdQueue.trypop(cmd)) {
        didsomething = true;
        theResourceManager.ProcessTaskCmd(cmd);
      }
    }

    {
      AssetChanges changes;
      while (theAssetManager.changesQueue.trypop(changes)) {
        didsomething = true;
        theAssetManager.SendAssetChanges(changes);
      }
    }

  } while (didsomething);


  // now just drop off the end (returning to main - which will also
  // drop off the end)
}
