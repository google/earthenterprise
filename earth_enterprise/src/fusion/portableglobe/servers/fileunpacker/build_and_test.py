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


"""Builds and tests Portable Server file unpacker library.

Builds the library for the 3 support systems: Mac, Linux
and Windows. Does basic tests to make sure that it can
read files and 3d and 2d packet data from globes and
maps.
"""

import distutils.sysconfig
import os
import platform
import re
import shutil
import subprocess
import sys
import util

# Add the OpenGEE Python libraries to the module search path:
opengee_lib_path = os.path.join(
  os.path.dirname(os.path.realpath(__file__)),
  '..', '..', '..', 'lib', 'python')

if opengee_lib_path not in sys.path:
  sys.path.insert(1, opengee_lib_path)

import opengee.c_compiler
import opengee.environ
import opengee.version


def build_swig(source_dir):
    opengee_build_dir = os.path.join(source_dir, 'NATIVE-REL-x86_64')
    swig_builder = subprocess.Popen('python2.7 /usr/bin/scons -j1 release=1 portable_3rd_libs=1 build', 
                                    shell=True, cwd=os.path.join(source_dir, "../"), env=os.environ.copy())
    if swig_builder.wait() != 0:
      raise ValueError('Swig build failed!')
    
    opengee.environ.env_prepend_path('PATH', os.path.join(opengee_build_dir, 'bin'), if_present='move')

def configure_c_compiler(os_dir):
  if os_dir != 'Linux':
    return
  version = opengee.c_compiler.get_cc_version('g++')
  if not version:
    raise ValueError('Unable to determine g++ version!')
  if not opengee.version.is_version_ge(version, [4, 8]):
    # Check for GCC 4.8 from the devtoolset-2-toolchain package on Red Hat 6:
    cc_dir = '/opt/rh/devtoolset-2/root/usr/bin'
    if os.path.isfile('{0}/g++'.format(cc_dir)):
      opengee.environ.env_prepend_path('PATH', cc_dir, if_present='move')
      opengee.environ.env_prepend_path(
        'LIBRARY_PATH',
        '/opt/rh/devtoolset-2/root/usr/lib',
        if_present='move'
      )
      opengee.environ.env_prepend_path(
        'LIBRARY_PATH',
        '/opt/rh/devtoolset-2/root/usr/lib64',
        if_present='move')
    else:
      raise ValueError('Version of g++ ({0}) is below minimum (4.8)!'.format(
          '.'.join(version)))

def BuildLibrary(os_dir, ignore_results, source_dir):
  """Returns whether able to build file unpacker library."""
  try:
    os.mkdir("dist")
  except OSError:
    pass  # ok if it already exists

  configure_c_compiler(os_dir)

  specialDefs = ''
  if os_dir == "Windows":
    # The logic below fixes a method used by the swig c++ wrapper. Mingw python headers
    # should detect and fix this but for some reason they aren't working with mingw64
    pythonCLib = "libpython{0}{1}.a".format(sys.version_info[0], sys.version_info[1])
    pathToLib = os.path.join(sys.exec_prefix, "libs", pythonCLib)
    if not os.path.isfile(pathToLib):
      print "ERROR: {0} was not found.  It is needed for linking".format(pathToLib)
      return False
    archData = platform.architecture(pathToLib)
    if archData[0] == "64bit":
      specialDefs = "-DMS_WIN64"
  elif os_dir == "Linux":
    build_swig(source_dir)

  os.chdir("dist")
  fp = open("../%s/build_lib" % os_dir)
  build_vars = {
    'prefix': sys.prefix,
    'exec_prefix': sys.exec_prefix,
    'python_inc_dir': distutils.sysconfig.get_python_inc(),
    'python_lib_dir': distutils.sysconfig.get_python_lib(),
    'special_defs': specialDefs
  }
  for line in fp:
    result = util.ExecuteCmd(line.format(**build_vars), use_shell=True)
    if result:
      if ignore_results:
        print result
      else:
        print "FAIL: %s" % result
        fp.close()
        return False
  fp.close()
  return True


def RunTests():
  """Runs tests using the generated library."""
  print "Running tests ..."
  shutil.copyfile("../util.py", "util.py")
  shutil.copyfile("../test.py", "test.py")

  old_path = sys.path
  sys.path = [os.getcwd()] + sys.path
  import test
  sys.path = old_path

  test.main()

def main(argv):
  """Main for build and test."""
  print(argv)
  if ((len(argv) < 2 or argv[1].lower() not in ["mac", "windows", "linux"]) or
      (len(argv) < 3 and arv[1].lower() == "linux")):
    print "Usage: build_and_test.py <OS_target> <source_dir>"
    print
    print "<OS_target> can be Mac, Windows, or Linux"
    print "<source_dir> is only needed for Linux"
    return

  os.chdir(os.path.dirname(os.path.realpath(__file__)))
  os_dir = "%s%s" % (argv[1][0:1].upper(), argv[1][1:].lower())

  print "Build and test file unpacker library for %s Portable Server." % os_dir

  if BuildLibrary(os_dir, argv[1].lower()=="windows", argv[2]):
    print "Library built."
    RunTests()
  else:
    sys.exit(1)


if __name__ == "__main__":
  main(sys.argv)
