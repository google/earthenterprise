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
    virtual bool ExecCmdline(Job *job, const std::vector<std::string> &cmdline) override {
      ++executes;
      return execPasses;
    }
    virtual void SendProgress(uint32 jobid, double progress, time_t progressTime) override {
      assert(progress == 0);
      ++progSent;
    }
    virtual void GetProcessStatus(pid_t pid, std::string* status_string,
                                  bool* success, bool* coredump, int* signum) override {
      ++getStatus;
      *success = statusPasses;
    }
    virtual void WaitForPid(pid_t waitfor, bool &success, bool &coredump,
                            int &signum) override {
      ++waitFors;
      success = statusPasses;
    }
    virtual void DeleteJob(
        std::vector<Job>::iterator which,
        bool success = false,
        time_t beginTime = 0, time_t endTime = 0) override {
      ++deletes;
      delSuccess = success;
      delTime = beginTime;
    }
    virtual Job* FindJobById(uint32 jobid, std::vector<Job>::iterator &found) override {
      return jobPointer;
    }
  public:
    // These variables modify how the class behaves
    Job myJob;
    Job * jobPointer;
    bool setLogFile;
    bool execPasses;
    bool statusPasses;

    // These variables record what the class does
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
        jobPointer(&myJob),
        setLogFile(true),
        execPasses(true),
        statusPasses(true),
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
    void RunJobLoop(bool multiCommands = false) {
      std::vector<std::vector<std::string>> commands;
      if (multiCommands) {
        commands = {{"cmd1"}, {"cmd2"}, {"cmd3"}};
      }
      else {
        commands = {{"cmd1"}};
      }
      JobLoop(StartJobMsg(1, "test.log", commands)); 
    }
};

class JobLoopTest : public testing::Test {
  protected:
    MockResourceProvider resProv;
};

TEST_F(JobLoopTest, NoJob) {
  resProv.jobPointer = nullptr;
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
  resProv.execPasses = false;
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
  resProv.statusPasses = false;
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
  resProv.statusPasses = false;
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

TEST_F(JobLoopTest, MultiCommandSuccess) {
  resProv.RunJobLoop(true);
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
  resProv.RunJobLoop(true);
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

/*
TODO:
- second FindJobById returns null
- multiple commands where one fails in ExecCmdline
- multiple commands where one fails in waitForPid
- multiple commands where one fails in GetProcessStatus
- no commands
- set begin time on first command
- locking behavior
*/

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
