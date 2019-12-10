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
    virtual void StartLogFile(Job * job, const std::string &logfile) override {}
    virtual void LogJobResults(
        Job * job,
        const std::string &status_string,
        int signum,
        bool coredump,
        bool success,
        time_t cmdtime,
        time_t endtime) override {}
    virtual void LogTotalTime(Job * job, uint32 elapsed) override {}

    virtual bool ExecCmdline(Job *job, const std::vector<std::string> &cmdline) override {
      ++executes;
      return execPasses;
    }
    virtual void SendProgress(uint32 jobid, double progress, time_t progressTime) override {
      ++progSent;
    }
    virtual void GetProcessStatus(pid_t pid, std::string* status_string,
                                  bool* success, bool* coredump, int* signum) override {
      ++getStatus;
    }
    virtual void WaitForPid(pid_t waitfor, bool &success, bool &coredump,
                            int &signum) override {
      ++waitFors;
    }
    virtual void DeleteJob(
        std::vector<Job>::iterator which,
        bool success = false,
        time_t beginTime = 0, time_t endTime = 0) override {
      ++deletes;
    }
    virtual Job* FindJobById(uint32 jobid, std::vector<Job>::iterator &found) override {
      return jobPointer;
    }
  public:
    // These variables modify how the class behaves
    Job myJob;
    Job * jobPointer;
    bool execPasses;

    // These variables record what the class does
    uint executes;
    uint progSent;
    uint getStatus;
    uint waitFors;
    uint deletes;

    MockResourceProvider() :
        myJob(1),
        jobPointer(&myJob),
        execPasses(true),
        executes(0),
        progSent(0),
        getStatus(0),
        waitFors(0),
        deletes(0) {}
    void RunJobLoop() { JobLoop(StartJobMsg(1, "test.log", {{"cmd1"}})); }
};

class JobLoopTest : public testing::Test {
  protected:
    MockResourceProvider resProv;
};

TEST_F(JobLoopTest, NoJob) {
  resProv.jobPointer = nullptr;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.executes, 0);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.deletes, 0);
}

TEST_F(JobLoopTest, ExecFails) {
  resProv.execPasses = false;
  resProv.RunJobLoop();
  ASSERT_EQ(resProv.executes, 1);
  ASSERT_EQ(resProv.progSent, 0);
  ASSERT_EQ(resProv.getStatus, 0);
  ASSERT_EQ(resProv.waitFors, 0);
  ASSERT_EQ(resProv.deletes, 1);
}

/*
TODO:
- multiple commands
- check SendProgress
- waitForPid/GetProcessStatus based on whether there's a log file
- second FindJobById returns null
- not successful
- successful
*/
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
