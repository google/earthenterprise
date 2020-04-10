/*
 * Copyright 2020 The Open GEE Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <set>

#include "khResourceProvider.h"

using namespace std;

const vector<vector<string>> NO_COMMANDS = {};
const vector<vector<string>> ONE_COMMAND = {{"cmd1"}};
const vector<vector<string>> MULTI_COMMANDS = {{"cmd1"}, {"cmd2"}, {"cmd3"}};

const unsigned int DEFAULT_TRIES = 3;

bool contains(const set< unsigned int>  & s, unsigned int v) {
  return s.find(v) != s.end();
}

class MockResourceProvider : public khResourceProvider {
  private:
    virtual void StartLogFile(JobIter job, const string &logfile) override {
      ++logStarted;
      if (setLogFile) {
        job->logfile = stdout;
      }
    }
    virtual void LogCmdResults(
        JobIter job,
        const string &status_string,
        int signum,
        bool coredump,
        bool success,
        time_t cmdtime,
        time_t endtime) override {
      ++resultsLogged;
    }
    virtual void LogTotalTime(JobIter job, std::uint32_t elapsed) override {
      ++timeLogged;
    }
    virtual bool ExecCmdline(JobIter job, const vector<string> &cmdline) override {
      ++executes;
      return !contains(failExecOn, executes);
    }
    virtual void SendProgress(std::uint32_t job, double progress, time_t progressTime) override {
      assert(progress == 0);
      ++progSent;
    }
    virtual void GetProcessStatus(pid_t pid, string* status_string,
                                  bool* success, bool* coredump, int* signum) override {
      ++getStatus;
      *success = !contains(failStatusOn, getStatus);
    }
    virtual void WaitForPid(pid_t waitfor, bool &success, bool &coredump,
                            int &signum) override {
      ++waitFors;
      success = !contains(failStatusOn, waitFors);
    }
    virtual void DeleteJob(
        JobIter which,
        bool success = false,
        time_t beginTime = 0, time_t endTime = 0) override {
      ++deletes;
      delSuccess = success;
      delTime = beginTime;
    }
    virtual JobIter FindJobById(std::uint32_t jobid) override {
      ++findJobs;
      return (findJobs == failFindJobOn ? myJob.end() : myJob.begin());
    }
    virtual inline bool Valid(JobIter job) const override { return job != myJob.end(); }
    virtual void LogRetry(JobIter job, unsigned int tries, unsigned int totalTries, unsigned int sleepBetweenTries) override {
      ++retriesLogged;
    }
  public:
    // These variables modify how the class behaves
    vector<Job> myJob;
    bool setLogFile;
    // Indicates which calls of each function should fail
    // If find job fails we abort immediately, so it only needs to fail once
    unsigned int failFindJobOn;
    // Other failures cause retries, so they may need to fail multiple times in
    // the tests (thus they are sets)
    set< unsigned int>  failExecOn;
    set< unsigned int>  failStatusOn;

    // These variables record what the class does
    unsigned int findJobs;
    unsigned int logStarted;
    unsigned int executes;
    unsigned int progSent;
    unsigned int getStatus;
    unsigned int waitFors;
    unsigned int resultsLogged;
    unsigned int timeLogged;
    unsigned int deletes;
    bool delSuccess;
    time_t delTime;
    unsigned int retriesLogged;

    MockResourceProvider() :
        myJob({1}),
        setLogFile(true),
        failFindJobOn(0),
        failExecOn(),
        failStatusOn(),
        findJobs(0),
        logStarted(0),
        executes(0),
        progSent(0),
        getStatus(0),
        waitFors(0),
        resultsLogged(0),
        timeLogged(0),
        deletes(0),
        delSuccess(false),
        delTime(0),
        retriesLogged(0) {}
    void RunJobLoop(const vector<vector<string>> & commands = ONE_COMMAND,
                    const unsigned int cmdTries = DEFAULT_TRIES,
                    const unsigned int sleepBetweenTriesSec = 0) {
      JobLoop(StartJobMsg(1, "test.log", commands), cmdTries, sleepBetweenTriesSec); 
    }
};

// Set the given failure to occur on all tries of a specific command.
void SetFailures(set< unsigned int>  & failOn, unsigned int cmd) {
  unsigned int startFail = cmd;
  unsigned int endFail = cmd + DEFAULT_TRIES;
  for (unsigned int i = startFail; i < endFail; ++i) {
    failOn.emplace(i);
  }
}

TEST(JobLoopTest, NoJob) {
  MockResourceProvider resProv;
  resProv.failFindJobOn = 1;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 0);
  ASSERT_EQ(resProv.executes, 0);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_EQ(resProv.deletes, 0);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, Success) {
  MockResourceProvider resProv;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, SuccessNoLogFile) {
  MockResourceProvider resProv;
  resProv.setLogFile = false;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 1);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, ExecFails) {
  MockResourceProvider resProv;
  SetFailures(resProv.failExecOn, 1);
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, DEFAULT_TRIES);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.retriesLogged, DEFAULT_TRIES - 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, GetStatusFails) {
  MockResourceProvider resProv;
  SetFailures(resProv.failStatusOn, 1);
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, DEFAULT_TRIES);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, DEFAULT_TRIES);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, DEFAULT_TRIES);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.retriesLogged, DEFAULT_TRIES - 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, WaitForPidFails) {
  MockResourceProvider resProv;
  SetFailures(resProv.failStatusOn, 1);
  resProv.setLogFile = false;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, DEFAULT_TRIES);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, DEFAULT_TRIES);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, FailSecondFindJob) {
  MockResourceProvider resProv;
  resProv.failFindJobOn = 2;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 0);
}

TEST(JobLoopTest, MultiCommandSuccess) {
  MockResourceProvider resProv;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 3);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 3);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 3);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, MultiCommandSuccessNoLog) {
  MockResourceProvider resProv;
  resProv.setLogFile = false;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 3);
  ASSERT_EQ(resProv.executes, 3);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 3);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, MultiCommandFailExec) {
  MockResourceProvider resProv;
  SetFailures(resProv.failExecOn, 2);
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1 + DEFAULT_TRIES);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.retriesLogged, DEFAULT_TRIES - 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, MultiCommandFailGetStatus) {
  MockResourceProvider resProv;
  SetFailures(resProv.failStatusOn, 2);
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1 + DEFAULT_TRIES);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1 + DEFAULT_TRIES);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1 + DEFAULT_TRIES);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.retriesLogged, DEFAULT_TRIES - 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, MultiCommandFailWaitFor) {
  MockResourceProvider resProv;
  SetFailures(resProv.failStatusOn, 2);
  resProv.setLogFile = false;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 2);
  ASSERT_EQ(resProv.executes, 1 + DEFAULT_TRIES);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 1 + DEFAULT_TRIES);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, MultiCommandFailFind) {
  MockResourceProvider resProv;
  resProv.failFindJobOn = 3;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 2);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_EQ(resProv.deletes, 0);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, NoCommands) {
  MockResourceProvider resProv;
  resProv.RunJobLoop(NO_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 0);
  ASSERT_EQ(resProv.executes, 0);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST(JobLoopTest, ExecFailsOnce) {
  MockResourceProvider resProv;
  resProv.failExecOn.emplace(1);
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 1);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, GetStatusFailsOnce) {
  MockResourceProvider resProv;
  resProv.failStatusOn.emplace(1);
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 2);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 2);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 1);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, WaitForPidFailsOnce) {
  MockResourceProvider resProv;
  resProv.failStatusOn.emplace(1);
  resProv.setLogFile = false;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 2);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 0);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, FailTwice) {
  const unsigned int FAILURES = 2;

  MockResourceProvider resProv;
  for (unsigned int i = 1; i <= FAILURES; ++i) {
    resProv.failExecOn.emplace(i);
  }
  resProv.RunJobLoop(ONE_COMMAND, FAILURES + 1);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 3);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_EQ(resProv.retriesLogged, 2);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST(JobLoopTest, SleepBetweenFails) {
  using TimePoint = chrono::time_point<std::chrono::high_resolution_clock>;

  const unsigned int SLEEP_TIME_SEC = 1;
  const unsigned int FAILURES = 2;

  MockResourceProvider resProv;
  for (unsigned int i = 1; i <= FAILURES; ++i) {
    resProv.failExecOn.emplace(i);
  }
  TimePoint start = chrono::high_resolution_clock::now();
  resProv.RunJobLoop(ONE_COMMAND, FAILURES + 1, SLEEP_TIME_SEC);
  TimePoint finish = chrono::high_resolution_clock::now();
  chrono::duration<double> elapsed = finish - start;
  ASSERT_GE(elapsed.count(), FAILURES * SLEEP_TIME_SEC);
  // Assume non-sleep processing takes less than a second
  ASSERT_LE(elapsed.count(), FAILURES * SLEEP_TIME_SEC + 1);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
