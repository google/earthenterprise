// Copyright 2018 The Open GEE Contributors
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

#include <gtest/gtest.h>
#include <string>
#include <time.h>

#include "performancelogger.h"

using namespace std;
using namespace testing;
using namespace getime;

#ifdef LOG_PERFORMANCE

using namespace performance_logger;

// Mock of the PerformanceLogger class
class MockPerformanceLogger {
  public:
    int calls;
    string operation;
    string object;
    timespec startTime;
    timespec endTime;
    size_t size;

    static MockPerformanceLogger& instance() {
      static MockPerformanceLogger _instance;

      return _instance;
    }
    void logTiming(
        const string & operation,
        const string & object,
        const timespec startTime,
        const timespec endTime,
        const size_t size = 0) {
      ++calls;
      this->operation = operation;
      this->object = object;
      this->startTime = startTime;
      this->endTime = endTime;
      this->size = size;
    }
    void reset() {
      calls = 0;
      operation = "";
      object = "";
      startTime.tv_sec = startTime.tv_nsec = 0;
      endTime.tv_sec = endTime.tv_nsec = 0;
      size = 0;
    }
  private:
    MockPerformanceLogger() {
      reset();
    }
};

// For convenience
MockPerformanceLogger& perfLogger = MockPerformanceLogger::instance();

class BlockPerformanceLoggerTest : public testing::Test {
  protected:
    virtual void SetUp() {
      // Reset the performance logger between each test
      perfLogger.reset();
    }
};

TEST_F(BlockPerformanceLoggerTest, Standard) {
  BlockPerformanceLogger<MockPerformanceLogger> logger("operation", "object", 123);
  EXPECT_EQ(perfLogger.calls, 0);
  logger.end();

  EXPECT_EQ(perfLogger.calls, 1);
  EXPECT_EQ(perfLogger.operation, "operation");
  EXPECT_EQ(perfLogger.object, "object");
  EXPECT_TRUE(perfLogger.startTime.tv_sec > 0 || perfLogger.startTime.tv_nsec > 0);
  EXPECT_TRUE(perfLogger.endTime.tv_sec > 0 || perfLogger.endTime.tv_nsec > 0);
  EXPECT_EQ(perfLogger.size, 123);
}

TEST_F(BlockPerformanceLoggerTest, NoSize) {
  BlockPerformanceLogger<MockPerformanceLogger> logger("operation", "object");
  EXPECT_EQ(perfLogger.calls, 0);
  logger.end();

  EXPECT_EQ(perfLogger.calls, 1);
  EXPECT_EQ(perfLogger.operation, "operation");
  EXPECT_EQ(perfLogger.object, "object");
  EXPECT_TRUE(perfLogger.startTime.tv_sec > 0 || perfLogger.startTime.tv_nsec > 0);
  EXPECT_TRUE(perfLogger.endTime.tv_sec > 0 || perfLogger.endTime.tv_nsec > 0);
  EXPECT_EQ(perfLogger.size, 0);
}

TEST_F(BlockPerformanceLoggerTest, OperationTiming) {
  BlockPerformanceLogger<MockPerformanceLogger> logger("op", "obj");
  sleep(3);
  logger.end();

  double duration = timespecToDouble(timespecDiff(perfLogger.endTime, perfLogger.startTime));
  EXPECT_GE(duration, 3.0);
  EXPECT_LT(duration, 4.0);
}

TEST_F(BlockPerformanceLoggerTest, MultiEnd) {
  BlockPerformanceLogger<MockPerformanceLogger> logger("operation", "object");
  EXPECT_EQ(perfLogger.calls, 0);
  logger.end();
  EXPECT_EQ(perfLogger.calls, 1);
  logger.end();
  EXPECT_EQ(perfLogger.calls, 1);
}

TEST_F(BlockPerformanceLoggerTest, LogOnDelete) {
  BlockPerformanceLogger<MockPerformanceLogger> * logger =
      new BlockPerformanceLogger<MockPerformanceLogger>("abc", "def", 987);
  EXPECT_EQ(perfLogger.calls, 0);
  delete logger;

  EXPECT_EQ(perfLogger.calls, 1);
  EXPECT_EQ(perfLogger.operation, "abc");
  EXPECT_EQ(perfLogger.object, "def");
  EXPECT_TRUE(perfLogger.startTime.tv_sec > 0 || perfLogger.startTime.tv_nsec > 0);
  EXPECT_TRUE(perfLogger.endTime.tv_sec > 0 || perfLogger.endTime.tv_nsec > 0);
  EXPECT_EQ(perfLogger.size, 987);
}

TEST_F(BlockPerformanceLoggerTest, DeleteAfterEnd) {
  BlockPerformanceLogger<MockPerformanceLogger> * logger =
      new BlockPerformanceLogger<MockPerformanceLogger>("abc", "def", 987);
  EXPECT_EQ(perfLogger.calls, 0);
  logger->end();
  EXPECT_EQ(perfLogger.calls, 1);
  logger->end();
  EXPECT_EQ(perfLogger.calls, 1);
  delete logger;
  EXPECT_EQ(perfLogger.calls, 1);
}

#endif  // LOG_PERFORMANCE

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
