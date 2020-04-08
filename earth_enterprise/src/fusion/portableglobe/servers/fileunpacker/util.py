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


"""Utility methods."""

import pexpect

# Try loading fdpexpect for version 3.* and above.
# if not successful, fallback to fdpexpect from older pexpect package.
try:
  import pexpect.fdpexpect as fdpexpect
except ImportError:
  import fdpexpect

import subprocess
import sys


class OsCommandError(Exception):
  """Thrown if os command fails."""
  pass

def PipePexpectStreamNonBlocking(source_stream, destination_stream):
  while source_stream.isalive():
    try:
      chunk = source_stream.read_nonblocking(timeout=0)
      destination_stream.write(chunk)
    except (pexpect.TIMEOUT, pexpect.EOF):
      return

def ExecuteCmd(os_cmd, use_shell=False):
  """Execute os command and log results."""
  print "Executing: {0}".format(os_cmd if use_shell else ' '.join(os_cmd))
  process = None
  try:
    process = subprocess.Popen(os_cmd, stdin=None, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, bufsize=0, shell=use_shell)
    stdout_stream = fdpexpect.fdspawn(process.stdout)
    stderr_stream = fdpexpect.fdspawn(process.stderr)
    # process.returncode is None until the subprocess completes. Then, it gets
    # filled in with the subprocess exit code.
    while process.returncode is None:
      PipePexpectStreamNonBlocking(stdout_stream, sys.stdout)
      PipePexpectStreamNonBlocking(stderr_stream, sys.stderr)
      process.poll()
    PipePexpectStreamNonBlocking(stdout_stream, sys.stdout)
    PipePexpectStreamNonBlocking(stderr_stream, sys.stderr)
    if process.returncode: # Assume a non-zero exit code means error:
      return "Unable to execute %s" % os_cmd
    return process.returncode
  except Exception, e:
    print "FAILED: %s" % e.__str__()
    raise OsCommandError()
  finally:
    # Terminate sub-process on keyboard interrupt or other exception:
    if process is not None and process.returncode is None:
      process.terminate()
