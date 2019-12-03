#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import json
import re
import sys
import StringIO
import time
import traceback
import os

# Need to use unittest2 for Python 2.6.
try:
  import unittest2 as unittest
except ImportError:
  import unittest

# Capture the error and out streams for use in test reports.
gee_err_stream = StringIO.StringIO()
gee_out_stream = StringIO.StringIO()

# Maximum number of lines in a traceback report.
MAX_TRACEBACK_SIZE = 5


class GeeTestRunner:
  """Simple runner that uses GeeTestResult to collect suite results."""

  def run(self, test):
    "Run the given test case or test suite."
    result = GeeTestResult(self)
    startTime = time.time()
    test(result)
    stopTime = time.time()
    timeTaken = float(stopTime - startTime)
    return result.results()


class GeeTestResult(unittest.TestResult):
  """Creates a list of test results for a suite."""

  def __init__(self, runner):
    unittest.TestResult.__init__(self)
    self.runner = runner
    self._results = []

  def addSuccess(self, test):
    result = self._BasicResult(test)
    result["status"] = "SUCCESS"
    self._results.append(result)

  def addError(self, test, error):
    result = self._BasicResult(test)
    result["status"] = "ERROR"
    self._AddErrorToResult(error, result)
    self._results.append(result)

  def addFailure(self, test, error):
    result = self._BasicResult(test)
    result["status"] = "FAILURE"
    self._AddErrorToResult(error, result)
    self._results.append(result)

  def addSkip(self, test, reason):
    result = self._BasicResult(test)
    result["status"] = "SKIPPED"
    result["skip_reason"] = reason
    self._results.append(result)

  def _AddErrorToResult(self, error, result):
    (error_type, error_value, error_traceback) = error
    result["error_type"] = error_type.__name__
    result["error_msg"] = error_value.message
    traceback_list = traceback.extract_tb(error_traceback, MAX_TRACEBACK_SIZE)
    result["error_traceback"] = "".join(traceback.format_list(traceback_list))

  def _BasicResult(self, test):
    return {
          "module": test.__module__,
          "class": test.__class__.__name__,
          "test": test._testMethodName,
          "description": test._testMethodDoc
        }

  def results(self):
    return self._results


def PrintJsonResults(results):
  """Print results in json format."""
  print json.dumps(results)


def PrintTextSuiteResults(results,
                          show_failure_messages=True,
                          show_stdout=True):
  """Print results for a test suite."""
  num_tests = 0
  num_successes = 0
  num_failures = 0
  num_errors = 0
  num_skipped = 0
  for result in results["test_results"]:
    print "-- (%s) %s %s" % (result["module"], result["test"], result["status"])
    if result["status"] == "SUCCESS":
      num_successes += 1
    else:
      if result["status"] == "FAILURE":
        num_failures += 1
        print "  %s: %s" % (result["error_type"], result["error_msg"])
        print "Traceback\n%s" % result["error_traceback"]
      elif result["status"] == "ERROR":
        num_errors += 1
        print "  %s: %s" % (result["error_type"], result["error_msg"])
        print "Traceback\n%s" % result["error_traceback"]
      elif result["status"] == "SKIPPED":
        num_skipped += 1
        print "  %s" % (result["skip_reason"])
      else:
        print "Unknown status:", result["status"]
        break
    num_tests += 1

  if show_stdout:
    print "Test messages:"
    for line in results["stdout"]:
      print line.strip()
  print "Summary: %d tests  %d successes  %d errors  %d failures %d skipped" % (
      num_tests, num_successes, num_errors, num_failures, num_skipped)


def PrintTextResults(results):
  """Print results for a test suite."""
  if results["user_tests"]:
    print "\n** User Tests **"
    PrintTextSuiteResults(results["user_tests"])
  if results["fusion_tests"]:
    print "\n** Fusion Tests **"
    PrintTextSuiteResults(results["fusion_tests"])
  if results["server_tests"]:
    print "\n** Server Tests **"
    PrintTextSuiteResults(results["server_tests"])


def PrintResults(results, format):
  """Print results in the given format."""
  if format.lower() == "json":
    PrintJsonResults(results)
  else:
    PrintTextResults(results)


def RunTests(test_runner, test_dir):
  """Run all tests in the given directory."""
  gee_err_stream.truncate(0)
  gee_out_stream.truncate(0)
  save_stdout = sys.stdout
  save_stderr = sys.stderr
  sys.stdout = gee_out_stream
  suite = unittest.TestLoader().discover(test_dir, pattern="*_test.py")
  results = {}
  results["test_results"] = test_runner.run(suite)
  sys.stdout = save_stdout
  sys.stderr = save_stderr
  gee_out_stream.seek(0)
  results["stdout"] = gee_out_stream.readlines()
  gee_err_stream.seek(0)
  results["stderr"] = gee_err_stream.readlines()
  return results


def Usage(app):
  print "Usage:"
  print ("  %s [--no_user_tests] [--no_fusion_tests] [--no_server_tests]"
         " [format]") % app
  print "     format - Output format ('json' or 'text'). Default: 'json'"
  exit(0)


def main(argv):
  """ Main function to run tests and render results. """
  for i in xrange(1, len(argv)):
    if argv[i] not in ["--no_user_tests",
                       "--no_fusion_tests",
                       "--no_server_tests",
                       "json",
                       "text"]:
      print "Unknown parameter:", argv[i]
      Usage(argv[0])

  test_runner = GeeTestRunner()

  results = {}

  user_test_dir = "geecheck_tests/user_tests"
  if "--no_user_tests" not in argv and os.path.isdir(user_test_dir):
    results["user_tests"] = RunTests(test_runner, user_test_dir)
  else:
    results["user_tests"] = None

  fusion_test_dir = "geecheck_tests/fusion_tests"
  if "--no_fusion_tests" not in argv and os.path.isdir(fusion_test_dir):
    results["fusion_tests"] = RunTests(test_runner, fusion_test_dir)
  else:
    results["fusion_tests"] = None

  server_test_dir = "geecheck_tests/server_tests"
  if "--no_server_tests" not in argv and os.path.isdir(server_test_dir):
    results["server_tests"] = RunTests(test_runner, server_test_dir)
  else:
    results["server_tests"] = None

  # Default to JSON if request is coming from a browser.
  if "REQUEST_METHOD" in os.environ:
    PrintResults(results, "json")
  else:
    if len(argv) > 1 and argv[-1] == "json":
      PrintResults(results, "json")
    else:
      PrintResults(results, "text")

if __name__ == "__main__":
  main(sys.argv)
