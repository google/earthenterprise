#!/usr/bin/python
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

"""Install Fusion and Earth Server, build tutorial databases, and test."""

import os
import subprocess

BASE_DIR = os.getcwd()
LOG_SHELL_CMDS = True
GEE_TESTS_LOG = "%s/gee_tests.log" % BASE_DIR

BYTES_PER_MEGABYTE = 1024.0 * 1024.0


class OsCommandError(Exception):
  """Thrown if os command fails."""
  pass


def BaseDir():
  """Returns the directory that contains the application that is running."""
  return BASE_DIR


def ClearLog():
  """Clear content of log file."""
  fp = open(GEE_TESTS_LOG, "w")
  fp.close()


def Log(message):
  """If logging is on, log the message."""
  if LOG_SHELL_CMDS:
    fp = open(GEE_TESTS_LOG, "a")
    fp.write(message + "\n")
    fp.close()


def ExecuteCmd(os_cmd, do_log=False, err2out=False):
  """Execute and log os command.

  If the shell command fails, an exception is thrown.

  Args:
    os_cmd: (string) linux shell command to run.
    do_log: whether to do logging.
    err2out: whether to send stderr to the same file handle as for stdout.
  Returns:
    results of the linux shell command.
  Raises:
    OsCommandError: if error from shell command is not None.
  """
  print "Executing: %s" % os_cmd
  if do_log:
    Log(os_cmd)

  try:
    p = subprocess.Popen(
        os_cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT if err2out else subprocess.PIPE)
    result, error = p.communicate()

    if (not err2out) and error:
      print "ERROR: %s" % error
      return "Unable to execute %s" % os_cmd

    return result
  except Exception, e:
    print "FAILED: %s" % e.__str__()
    raise OsCommandError()


def DiskSpace(path):
  """Returns remaining disk space in Megabytes."""
  mount_info = os.statvfs(path)
  return mount_info.f_bsize * mount_info.f_bavail / BYTES_PER_MEGABYTE


def ChDir(path):
  """Changes directory so that it is logged."""
  Log("cd %s" % path)
  os.chdir(path)


def GetFileWithReplace(path, replace):
  """Return content of file after replacing any keys in replace with values."""
  fp = open(path)
  content = fp.read()
  fp.close()
  for key in replace.iterkeys():
    content = content.replace(key, replace[key])
  return content


def main():
  # Quick test of module.
  ClearLog()
  print ExecuteCmd("pwd")
  ChDir("/tmp")
  print ExecuteCmd("pwd")
  ChDir(BASE_DIR)
  print ExecuteCmd("pwd")

if __name__ == "__main__":
  main()
