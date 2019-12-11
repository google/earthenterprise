/*
 * Copyright 2019 The Open GEE Contributors
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

#include "khResourceProvider.h"

const std::vector<std::vector<std::string>> NO_COMMANDS = {};
const std::vector<std::vector<std::string>> ONE_COMMAND = {{"cmd1"}};
const std::vector<std::vector<std::string>> MULTI_COMMANDS = {{"cmd1"}, {"cmd2"}, {"cmd3"}};

class MockResourceProvider : public khResourceProvider {
  private:
    virtual void StartLogFile(Job * job, const std::string &logfile) override {
      ++logStarted;
      if (setLogFile) {
        job->logfile = stdout;
      }
    }
    virtual void LogJobResults(
        Job * job,
        const std::string &status_string,
        int signum,
        bool coredump,
        bool success,
        time_t cmdtime,
        time_t endtime) override {
      ++resultsLogged;
    }
    virtual void LogTotalTime(Job * job, uint32 elapsed) override {
      ++timeLogged;
    }
    virtual bool ExecCmdline(Job * job, const std::vector<std::string> &cmdline) override {
      ++executes;
      return !(executes == failExecOn);
    }
    virtual void SendProgress(Job * job, double progress, time_t progressTime) override {
      assert(progress == 0);
      ++progSent;
    }
    virtual void GetProcessStatus(pid_t pid, std::string* status_string,
                                  bool* success, bool* coredump, int* signum) override {
      ++getStatus;
      *success = !(getStatus == failStatusOn);
    }
    virtual void WaitForPid(pid_t waitfor, bool &success, bool &coredump,
                            int &signum) override {
      ++waitFors;
      success = !(waitFors == failStatusOn);
    }
    virtual void DeleteJob(
        JobIter which,
        bool success = false,
        time_t beginTime = 0, time_t endTime = 0) override {
      ++deletes;
      delSuccess = success;
      delTime = beginTime;
    }
    virtual Job* FindJobById(uint32 jobid, JobIter &found) override {
      ++findJobs;
      return (findJobs == failFindJobOn ? nullptr : &myJob);
    }
  public:
    // These variables modify how the class behaves
    Job myJob;
    bool setLogFile;
    // Indicates which call of each function should fail
    uint failFindJobOn;
    uint failExecOn;
    uint failStatusOn;

    // These variables record what the class does
    uint findJobs;
    uint logStarted;
    uint executes;
    uint progSent;
    uint getStatus;
    uint waitFors;
    uint resultsLogged;
    uint timeLogged;
    uint deletes;
    bool delSuccess;
    time_t delTime;

    MockResourceProvider() :
        myJob(1),
        setLogFile(true),
        failFindJobOn(0),
        failExecOn(0),
        failStatusOn(0),
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
        delTime(0) {}
    void RunJobLoop(const std::vector<std::vector<std::string>> & commands = ONE_COMMAND) {
      JobLoop(StartJobMsg(1, "test.log", commands)); 
    }
};

class JobLoopTest : public testing::Test {
  protected:
    MockResourceProvider resProv;
};

TEST_F(JobLoopTest, NoJob) {
  resProv.failFindJobOn = 1;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 0);
  ASSERT_EQ(resProv.executes, 0);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 0);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, Success) {
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST_F(JobLoopTest, SuccessNoLogFile) {
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
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST_F(JobLoopTest, ExecFails) {
  resProv.failExecOn = 1;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, GetStatusFails) {
  resProv.failStatusOn = 1;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 1);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, WaitForPidFails) {
  resProv.failStatusOn = 1;
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
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, FailSecondFindJob) {
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

TEST_F(JobLoopTest, MultiCommandSuccess) {
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 3);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 3);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 3);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST_F(JobLoopTest, MultiCommandSuccessNoLog) {
  resProv.setLogFile = false;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 3);
  ASSERT_EQ(resProv.executes, 3);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 3);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_TRUE(resProv.delSuccess);
  ASSERT_NE(resProv.delTime, 0);
}

TEST_F(JobLoopTest, MultiCommandFailExec) {
  resProv.failExecOn = 2;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 1);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, MultiCommandFailGetStatus) {
  resProv.failStatusOn = 2;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 2);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 2);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, MultiCommandFailWaitFor) {
  resProv.failStatusOn = 2;
  resProv.setLogFile = false;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 2);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 2);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 1);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, MultiCommandFailFind) {
  resProv.failFindJobOn = 3;
  resProv.RunJobLoop(MULTI_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 1);
  ASSERT_EQ(resProv.executes, 2);
  ASSERT_EQ(resProv.progSent, 1);
  ASSERT_EQ(resProv.getStatus, 2);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 1);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 0);
  ASSERT_FALSE(resProv.delSuccess);
}

TEST_F(JobLoopTest, NoCommands) {
  resProv.RunJobLoop(NO_COMMANDS);
  ASSERT_EQ(resProv.logStarted, 0);
  ASSERT_EQ(resProv.executes, 0);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.resultsLogged, 0);
  ASSERT_EQ(resProv.timeLogged, 0);
  ASSERT_EQ(resProv.deletes, 1);
  ASSERT_FALSE(resProv.delSuccess);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
