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
#

"""Runs a series of tests of the Portable server independent of OS.

Relies on a set of gold standard files in the tests directory. Converts each
url to a unique file path where it looks for the gold data for the current
globe or map. If the output_gold flag is set, then it saves the the results
to the gold data file rather than comparing to it. This can be used to
help generate a new set of gold standard files.

The test file will look something like:

globes:
test_globe.glb
test_map.glm

tests:
change_globe test_map.glm
maps/map.json
query?request=ImageryMaps&channel=10&&x=52&y=12&z=7
change_globe test_globe.glb
flatfile?f1c-02121-t.6&ct=p
EOF

There is also an "include" syntax that allows tests to be broken up into
manageable subcomponents. E.g.

globes:
test_globe.glb
test_map.glm

tests:
include[
test_test_globe.glb.txt
test_test_map.glm.txt
]
EOF

The globes tag indicates the globes and maps needed to run the tests.
The tests consist of commands, which allow operations such as changing
the served item to a map or globe, and the tests themselves, which
consist of comparing what is returned for a given url from the server to
the contents of a gold standard file.
"""

import md5
import optparse
import os
import urllib

TESTS_FILE_TEMPLATE = "%s/tests.txt"

# States for parsing tests
STATE_NONE = 0
STATE_GLOBES = 1
STATE_TESTS = 2
STATE_INCLUDE = 3

# Commands
COMMANDS_CHANGE_GLOBE = "change_globe"
COMMANDS_BROADCAST = "broadcast"

# Tags
TAGS_GLOBES = "globes:"
TAGS_TESTS = "tests:"
TAGS_INCLUDE = "include["
TAGS_END_INCLUDE = "]"

# PRESET URLS
LAPTOP_PING_URL = "ping"
EARTHSERVER_PING_URL = "statusz"


class Tester(object):
  """Class for running tests on Portable."""

  def __init__(self, params):
    """Constructor."""
    if params.test_dir[0] == "/":
      self.tests_dir_ = params.test_dir
    else:
      self.tests_dir_ = "%s/%s" % (os.getcwd(), params.test_dir)
    self.tests_file_ = TESTS_FILE_TEMPLATE % self.tests_dir_
    self.gold_dir_ = params.gold_dir
    self.output_gold_ = params.output_gold
    self.platform_ = params.platform
    self.server_ = params.server
    self.current_globe_ = "unknown"
    self.ready = self._ParseTests(self.tests_file_)

  def CheckPortableServer(self):
    """Makes sure Portable server is running."""
    ping_url = LAPTOP_PING_URL
    if self.platform_ == "earthserver":
      ping = self._GetUrl(EARTHSERVER_PING_URL)
      if not "globe directory:" in ping:
        self.Error("Server may not be running.")
        return False
    elif not self._TestUrl(ping_url):
      self.Error("Server may not be running.")
      return False
    print "Check Portable server."
    return True

  def CheckGlobes(self):
    """Makes sure that all necessary globes exist on the server."""
    print "Check globes."
    for globe in self.globes_:
      if not os.path.exists(self._GlobePath(globe)):
        self.Failure("No such globe: %s" % globe)
        return False
    return True

  def RunTests(self):
    """Runs all of the tests."""
    for next_test in self.tests_:
      if "command" in next_test:
        if next_test["command"] == COMMANDS_CHANGE_GLOBE:
          self._ChangeGlobe(next_test["argument"])
        elif next_test["command"] == COMMANDS_BROADCAST:
          self._Broadcast(next_test["argument"])
        else:
          self.Error("Unknown command when running tests.")
      else:
        url = next_test["url"]
        if not self._TestUrl(url):
          return
    print "**SUCCESS**"

  def Error(self, message):
    """Output error message in consistent manner."""
    print "**ERROR**: %s" % message

  def Failure(self, message):
    """Output failure message in consistent manner."""
    print "**FAILURE**: %s" % message

  def Warning(self, message):
    """Output failure message in consistent manner."""
    print "**WARNING**: %s" % message

  def _ChangeGlobe(self, globe_name):
    """Ask server to serve the given globe."""
    print "change globe", globe_name
    self.current_globe_ = globe_name
    if self.platform_ == "earthserver":
      return

    # TODO: Make specific to platform
    url = "?cmd=serve_globe&globe=%s" % self._GlobePath(globe_name)
    self._GetUrl(url)

  def _GlobePath(self, globe_name):
    """Returns the globe path."""
    return "%s/globes/%s" % (self.tests_dir_, globe_name)

  def _Broadcast(self, state):
    """Ask server to broadcast or to stop broadcasting depending on state."""
    self.Warning("Broadcast not yet implemented. Can't set it to %s." % state)

  def _ParseTests(self, tests_file):
    """Loads and parses the tests from a text file."""
    fp = open(tests_file)
    self.globes_ = []
    self.tested_globes_ = []
    self.tests_ = []
    self.state_ = STATE_NONE
    for line in fp:
      if not self._ParseLine(line):
        fp.close()
        return False
    fp.close()
    for globe in self.globes_:
      if not globe in self.tested_globes_:
        self.Warning("Globe %s is not tested." % globe)
    return True

  def _ParseLine(self, line):
    line = line.strip()
    if not line or line[0] == "#":
      # Allow comments and blank lines.
      pass
    else:
      if line == TAGS_GLOBES:
        self.state_ = STATE_GLOBES
      elif line == TAGS_TESTS:
        self.state_ = STATE_TESTS
      elif line == TAGS_INCLUDE:
        self.include_state_ = self.state_
        self.state_ = STATE_INCLUDE
      elif line == TAGS_END_INCLUDE:
        self.state_ = self.include_state_
      else:
        if self.state_ == STATE_INCLUDE:
          include_file = "%s/%s" % (self.tests_dir_, line)
          if not os.path.exists(include_file):
            self.Error("Unable to find test file: %s" % include_file)
            return False
          fp = open(include_file)
          self.state_ = self.include_state_
          for line in fp:
            if not self._ParseLine(line):
              fp.close()
              return False
          fp.close()
          self.include_state_ = self.state_
          self.state_ = STATE_INCLUDE

        elif self.state_ == STATE_GLOBES:
          self.globes_.append(line)

        elif self.state_ == STATE_TESTS:
          good_test = self._ParseTest(line)
          if not good_test:
            return False

        else:
          self.Error("State has not yet been set.")
          return False
    return True

  def _ConvertUrlToPath(self, url):
    """Create path to gold data."""
    if not "?" in url:
      path = url
      file_name = "noargs"
    else:
      (path, args) = url.split("?")
      file_name = md5.new(args).hexdigest()
    full_path = "%s/%s/%s" % (self.gold_dir_, self.current_globe_, path)
    full_path.replace("//", "/")
    full_file_path = "%s/%s" % (full_path, file_name)
    full_file_path.replace("//", "/")
    return (full_path, full_file_path)

  def _ParseTest(self, test):
    """Parses a test line from the tests file."""
    args = test.split(" ")
    next_test = {}
    if len(args) > 2:
      self.Error("Bad test line: %s\n" % test)
      return False

    # Commands: command arg
    elif len(args) == 2:
      if args[0] == COMMANDS_CHANGE_GLOBE:
        next_test["command"] = args[0]
        globe = args[1]
        next_test["argument"] = globe
        if not globe in self.globes_:
          self.Error("Unknown globe: %s" % globe)
          return False
        else:
          self.tested_globes_.append(globe)

      elif args[0] == COMMANDS_BROADCAST:
        next_test["command"] = args[0]
        argument = args[1]
        next_test["argument"] = argument
        if not argument in ["on", "off"]:
          self.Error("Expected argument 'on' or 'off', not '%s'." % argument)
          return False

      else:
        self.Error("Unknown command: '%s'" % args[0])
        return False

    # Test server: url
    else:
      next_test["url"] = args[0]

    self.tests_.append(next_test)
    return True

  def _TestUrl(self, url):
    """Compare result from the url to the contents of the given file."""
    print "Test", url
    (path, file_name) = self._ConvertUrlToPath(url)
    if self.platform_ == "earthserver":
      full_url_path = "%s/%s" % (self.current_globe_, url)
    else:
      full_url_path = url
    data = self._GetUrl(full_url_path)
    # Write data to gold file
    if self.output_gold_:
      if not data:
        self.Warning("No data from: %s" % url)

      try:
        try:
          os.makedirs(path)
        except:
          # Ok if directory already exists
          pass

        print "Writing", file_name
        fp = open(file_name, "wb")
        fp.write(data)
        fp.close()
      except Exception as e:
        self.Error("Unable to write file: %s\n%s" % (file_name, e))
        return False
      return True
    # Compare data against gold file
    else:
      try:
        fp = open(file_name, "rb")
        expected_data = fp.read()
        fp.close()
      except Exception as e:
        self.Error("Unable to read file: %s" % file_name)
        return False

      if expected_data != data:
        # Show first mismatched line
        expected = expected_data.split("\n")
        actual = data.split("\n")
        # This must explain the inequality if all lines match.
        message = "Actual contains  more data than expected."
        for i in xrange(len(expected)):
          if expected[i] != actual[i]:
            message = ("\nExpected %s:\n%s\nActual %s:\n%s"
                       % (file_name, expected[i], url, actual[i]))
            break
        self.Failure(message)
        return False
      return True

  def _GetUrl(self, url):
    """Get content from the given url."""
    try:
      url = "%s/%s" % (self.server_, url)
      fp = urllib.urlopen(url)
      content = fp.read()
      fp.close()
    except Exception as e:
      print e
      return None

    return content


def ParseArgs():
  """Parse command line arguments."""
  parser = optparse.OptionParser()
  parser.add_option("-t", "--test_directory", dest="test_dir",
                    help=("Test directory containing file specifying "
                          "tests to be run. All relative paths in "
                          "test specification are relative to this "
                          "directory."),
                    default="tests")
  parser.add_option("-s", "--server", dest="server",
                    help="Portable server to test.",
                    default="http://localhost:9335")
  parser.add_option("-g", "--gold_directory", dest="gold_dir",
                    help="Directory of gold data.",
                    default="tests/gold")
  parser.add_option("-p", "--platform", dest="platform",
                    help=("Platform that the Portable server is running on "
                          " (laptop, android, earthserver)."),
                    default="laptop")
  parser.add_option("-o", "--output_gold", action="store_true",
                    dest="output_gold",
                    help=("Output to gold directory "
                          "rather than comparing to it."))
  return parser.parse_args()


def main():
  """Run all tests."""
  (params, unused_) = ParseArgs()
  tester = Tester(params)
  if tester.ready:
    if not tester.CheckPortableServer():
      return
    if not tester.CheckGlobes():
      return
    tester.RunTests()
  else:
    tester.Error("Failed to parse tests.")

if __name__ == "__main__":
  main()
