/*
 * Copyright 2017 Google Inc.
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

//
// Base class for unit tests

#ifndef COMMON_UNITTEST_H__
#define COMMON_UNITTEST_H__

#include <vector>
#include <string>
#include <utility>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <khTimer.h>


// ****************************************************************************
// ***  TestAsset
// ***
// ***  Like assert, but doesn't compile out
// ***
// ***  Borrowed from assert.h
// ****************************************************************************
#ifndef __ASSERT_FUNCTION
#   define __ASSERT_FUNCTION    __PRETTY_FUNCTION__
#endif

extern "C" void __assert_fail (__const char *__assertion, __const char *__file,
			   unsigned int __line, __const char *__function)
     __THROW __attribute__ ((__noreturn__));

# define TestAssert(expr) \
  (__ASSERT_VOID_CAST ((expr) ? 0 : \
		       (__assert_fail (__STRING(expr), __FILE__, __LINE__, \
				       __ASSERT_FUNCTION), 0)))
# define TestAssertEquals(x, y) \
  (TestEquals(x, y) ? 0 : \
		       (__assert_fail (__STRING(x == y), __FILE__, __LINE__, \
				       __ASSERT_FUNCTION), 0))

# define TestAssertEqualsWithinEpsilon(x, y, epsilon) \
  (TestEqualsWithinEpsilon(x, y, epsilon) ? 0 : \
		       (__assert_fail (__STRING(x == y +- epsilon), __FILE__, __LINE__, \
				       __ASSERT_FUNCTION), 0))
# define TestAssertWithMessage(expr, message) \
  (__ASSERT_VOID_CAST ((expr) ? 0 : \
		       (__assert_fail (message.c_str(), __FILE__, __LINE__, \
				       __ASSERT_FUNCTION), 0)))

#ifndef CHECK
#define CHECK(a) TestAssert(TestExpect((a), #a, __FILE__, __LINE__))
#endif

#define CHECK_EQ(a, b) \
  TestAssert(TestExpectEq((a), #a, (b), #b, __FILE__, __LINE__))
#define CHECK_STREQ(a, b) TestAssert(0 == strcmp((a), (b)))
#define EXPECT(a) TestExpect((a), #a, __FILE__, __LINE__)
#define EXPECT_EQ(a, b) TestExpectEq((a), #a, (b), #b, __FILE__, __LINE__)
// We define emulations of some convenient gUnit macros here.  Use:
// create "bool success = true;" at start of test, "return success;"
// at end.
#define EXPECT_TRUE(a) EXPECT(a)
#define EXPECT_FALSE(a) EXPECT(!(a))

/****************************************************************************
 ***  UnitTest
 ***
 ***  Base class for unit test objects.
 ***
 ***  --- myunittest.cpp ---
 ***  #include <UnitTest.h>
 ***
 ***  class MyUnitTest : public UnitTest<MyUnitTest> {
 ***   public:
 ***    bool test1(void) {
 ***      blah blah blah;
 ***      return true;
 ***    }
 ***    bool test2(void) {
 ***      blah blah blah;
 ***      return false;
 ***    }
 ***    MyUnitTest(void) : BaseClass("MyUnitTest") {
 ***      REGISTER(test1);
 ***      REGISTER(test2);
 ***    }
 ***  };
 ***
 ***  int main(int argc, char *argv[]) {
 ***    MyUnitTest tests;
 ***    return tests.Run();
 ***  }
 ***  ----------------------
 ***
 ***  ---- sample output ---
 ***  test1 succeeded
 ***  test2 FAILED
 ***  MyUnitTest unit tests FAILED (1 of 2)
 ***  ----------------------
 ****************************************************************************/

template <class Derived>
class UnitTest {
  std::string name;
 public:
  typedef UnitTest<Derived> BaseClass;
  typedef Derived DerivedUnitTest;
  typedef bool (Derived::*TestFunc)(void);

  // Utility struct class to manage the information needed for each
  // test.
  class TestFunctionInfo {
  public:
    std::string name;  // Name of the function (for output).
    TestFunc func;
    int execute_n_times;  // Execute the function "n" times
                          // (for timing tests only).
    double max_secs;  // Maximum seconds for the total execution time (all n).
    TestFunctionInfo(std::string name_in, TestFunc func_in,
                     int execute_n_times_in, double max_secs_in) {
      name = name_in;
      func = func_in;
      execute_n_times = execute_n_times_in;
      max_secs = max_secs_in;
    }
  };
  typedef std::vector<TestFunctionInfo> VectorType;
  typedef typename VectorType::iterator VectorIterator;
  VectorType funcs;
  VectorType timed_funcs;
  bool debug_; // Set to true to enable debug options like printing etc.
  bool inner_test_result_;  // Keep track of the success of all "TestXXX" calls
                            // within a single Test function.
                            // Allows TestExcept to work without additional
                            // effort, while still noting the test failure.

  UnitTest(const std::string &name_) : name(name_) {
   debug_ = false;
  }

  int Run(void) {
    Derived *self = static_cast<Derived*>(this);

    // We will run two groups of tests, regular unit tests and timed unit tests.
    int passed = 0;
    // Run the regular unit tests.
    for (VectorIterator func = funcs.begin(); func != funcs.end(); ++func) {
      inner_test_result_ = true;  // Reset this before each test run.
                                  // The Test may return true, but if any
                                  // TestExcept of other failures occur, we'll
                                  // still mark it as failed as
                                  // inner_test_result_ will keep track of that.
      bool result = (self->*(func->func))() & inner_test_result_;
      fprintf(stderr, "%s %s\n", func->name.c_str(),
              result ? "succeeded" : "FAILED");
      if (result) {
        ++passed;
      }
    }

    // Run the timed unit tests.
    int i = 0;
    for (VectorIterator func = timed_funcs.begin();
          func != timed_funcs.end(); ++func, ++i) {
      bool result = TimedFunction(func->func, func->name,
                                  func->execute_n_times, func->max_secs);
      if (result) {
        ++passed;
      }
    }

    int total_tests = (int)(funcs.size() + timed_funcs.size());
    if (passed == total_tests) {
      fprintf(stderr, "%s unit tests succeeded (%d of %d)\n",
              name.c_str(), passed, total_tests);
      return 0;
    } else {
      fprintf(stderr, "%s unit tests FAILED (%d of %d)\n",
              name.c_str(), total_tests - passed, total_tests);
      return 2;
    }
  }

  // Utility method for timing a function.
  // Prints out time the test in seconds.
  // If the timing takes longer than max_secs return false.
  // set max_secs <= 0 to disable the time test.
  bool TimedFunction(bool (DerivedUnitTest::*func)(), std::string function_name,
                     int execute_n_times, double max_secs) {
    Derived *self = static_cast<Derived*>(this);
    // Disable debugging for any timing tests.
    bool debug_temp = debug_;
    debug_ = false;

    khTimer_t start = khTimer::tick();
    // Execute the function...ignore the result...we're not testing
    // correctness here.
    for(int i = 0; i < execute_n_times; ++i) {
      ((*self).*func)();
    }
    khTimer_t stop = khTimer::tick();
    double secs = khTimer::delta_s(start, stop);

    // Reset the debug_ value.
    debug_ = debug_temp;

    // All timing tests echo the timing and success/fail if relevant
    std::cerr << "Timed: " << function_name << "[" << execute_n_times
      << "]: " << secs << " secs";
    // Check for the timing test if requested.
    bool result = true;
    if (max_secs > 0) {
      if (secs > max_secs) {
        std::cerr << " FAILED expected < " << max_secs;
        result = false;
      } else {
        std::cerr << " succeeded";
      }
    }
    std::cerr << std::endl;
    return result;
  }

  // Equality test
  template <typename T> bool TestEquals(T x, T y) {
    if (x == y) return true;
    std::cerr << " FAILED TestEquals: " <<
      ", actual values: [" << x << ", " << y << "]" << std::endl;
    inner_test_result_ = false;
    return false;
  }
  // Equality test with epsilon
  template <typename T> bool TestEqualsWithinEpsilon(T x, T y, T epsilon) {
    T dxy = x - y;
    if (dxy < 0) dxy = - dxy;
    if (dxy <= epsilon) return true;
    std::streamsize precision = std::cerr.precision(8);
    std::cerr << " FAILED TestEqualsWithinEpsilon;" <<
      ", actual values: [" << x << ", " << y << "] delta=" << dxy <<  std::endl;
    std::cerr.precision(precision);
    inner_test_result_ = false;
    return false;
  }

  // TestExpect will return false if "test_result" is false and print
  // the specified error message to stderr.
  // If the test_result is true it will return true.
  bool TestExpect(const bool test_result,
                  const std::string& test_string,
                  const std::string& filename,
                  int line) {
    if (test_result) {
      return true;
    } else {
      std::cerr << "ERROR: \"" << test_string << "\" is false at " << std::endl
                << "  " << filename << ":" << line << std::endl;
      inner_test_result_ = false;
      return false;
    }
  }

  // TestExpectEq compares two values and returns true if they are "==".
  // If not "==" it will print the desired message to stderr and return false.
  template <class Ta, class Tb> bool TestExpectEq(const Ta& a,
                                                  const std::string &a_str,
                                                  const Tb& b,
                                                  const std::string &b_str,
                                                  const std::string &filename,
                                                  int line) {
    if (a == b) {
      return true;
    } else {
      std::cerr << "ERROR: expected \"" << a_str << "\" == \"" << b_str
        << std::endl;
      std::cerr << "       actual  \"" << a_str << "\" = " << a
                << " , \"" << b_str << "\" = " << b << std::endl;
      std::cerr << "\" at " << std::endl << "  " << filename << ":" << line
        << std::endl;
      inner_test_result_ = false;
      return false;
    }
  }
};

// Register a function for a the unit test.
#define REGISTER(func) \
funcs.push_back( \
  DerivedUnitTest::TestFunctionInfo(std::string(#func), \
                                    &DerivedUnitTest::func, 1, 0.0))

// Add a timing test which will run "func" "n" times and print the timing stats.
// If max_secs > 0 AND the timing (total of all "n" runs)
// is greater than max_secs, the test will fail .
#define REGISTER_TIMED_TEST(func, execute_n_times, max_secs) \
timed_funcs.push_back( \
  DerivedUnitTest::TestFunctionInfo(std::string(#func), \
                                    &DerivedUnitTest::func, \
                                    execute_n_times, max_secs))


#endif // COMMON_UNITTEST_H__
