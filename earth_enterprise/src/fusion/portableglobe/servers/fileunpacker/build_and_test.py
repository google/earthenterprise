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


"""Builds and tests Portable Server file unpacker library.

Builds the library for the 3 support systems: Mac, Linux
and Windows. Does basic tests to make sure that it can
read files and 3d and 2d packet data from globes and
maps.
"""

import distutils.sysconfig
import os
import re
import shutil
import subprocess
import sys
import util


def get_cc_version(cc_path):
  """Extracts the version number of a C/C++ compiler (like GCC) as a list of
  strings.  (Like ['5', '4', '0'].)"""

  process = subprocess.Popen(
    [cc_path, '--version'], stdout=subprocess.PIPE)
  (stdout, _) = process.communicate()
  match = re.search('[0-9][0-9.]*', stdout)

  # If match is None we don't know the version.
  if match is None:
    return None

  version = match.group().split('.')

  return version

def is_version_ge(version_components, comparand_components):
  """Tests if a version number is greater than, or equal to another version
  number.  Both version numbers are specified as lists of version components.
  """

  if version_components is None or comparand_components is None:
    return False

  for i in xrange(len(comparand_components)):
    if i >= len(version_components):
      version_component = 0
    else:
      version_component = int(version_components[i])
    if int(comparand_components[i]) > version_component:
      return False
    if int(comparand_components[i]) < version_component:
      return True

  return True

def path_prepend(value, new_path, if_present='skip', separator=':'):
  """Adds a component to a PATH variable, where paths are separated by
  `separator`.

    if_present = What to do, if PATH already contains `new_path`.
      'skip': leave PATH as is.
      'move': move `new_path` to the beginning of PATH.
    separator = String that separates path components in the variable value.
  """

  # Check if the PATH variable already contains `new_path`:
  if value == new_path:
    return value
  if value.startswith(new_path + separator):
    if if_present == 'skip' or 'move':
      return value
    else:
      raise ValueError('Invalid if_present argument: {0}'.format(if_present))
  if value.endswith(separator + new_path):
    if if_present == 'skip':
      return value
    elif if_present == 'move':
      return new_path + separator + value[ : -len(new_path) - 1]
    else:
      raise ValueError('Invalid if_present argument: {0}'.format(if_present))
  i = value.find(separator + new_path + separator)
  if i >= 0:
    if if_present == 'skip':
      return value
    elif if_present == 'move':
      return new_path + separator + value[: i] + value[i + len(new_path) + 1:]
    else:
      raise ValueError('Invalid if_present argument: {0}'.format(if_present))

  # PATH doesn't contain `new_path`, prepend it:
  if value == '':
    return new_path

  return new_path + separator + value

def environ_var_path_prepend(
  var_name, new_path, if_present='skip', separator=':'
):
  """Prepend a path component to a PATH-like environment variable (i.e., one
  which contains paths separated by `separator`).

    var_name = Name of the environment variable to add a path component to.
  The variable is created, if it doesn't exist.
    new_path = Path to add to the variable value.
    if_present = What to do, if the variable already contains `new_path`.
      'skip': Leave the variable value as is.
      'move': Move `new_path` to the beginning of the variable value.
    separator = String that separates path components in the variable value.
  """

  try:
    value = os.environ[var_name]
  except KeyError:
    value = ''
  os.environ[var_name] = path_prepend(
    value, new_path, if_present=if_present, separator=separator)

def configure_c_compiler(os_dir):
  if os_dir != 'Linux':
    return
  version = get_cc_version('g++')
  if not version:
    raise ValueError('Unable to determine g++ version!')
  if not is_version_ge(version, [4, 8]):
    # Check for GCC 4.8 from the devtoolset-2-toolchain package on Red Hat 6:
    cc_dir = '/opt/rh/devtoolset-2/root/usr/bin'
    if os.path.isfile('{0}/g++'.format(cc_dir)):
      environ_var_path_prepend('PATH', cc_dir, if_present='move')
      environ_var_path_prepend(
        'LIBRARY_PATH',
        '/opt/rh/devtoolset-2/root/usr/lib',
        if_present='move'
      )
      environ_var_path_prepend(
        'LIBRARY_PATH',
        '/opt/rh/devtoolset-2/root/usr/lib64',
        if_present='move')
    else:
      raise ValueError('Version of g++ ({0}) is below minimum (4.8)!'.format(
          '.'.join(version)))

def BuildLibrary(os_dir, ignore_results):
  """Returns whether able to build file unpacker library."""
  try:
    os.mkdir("dist")
  except OSError:
    pass  # ok if it already exists

  configure_c_compiler(os_dir)

  os.chdir("dist")
  fp = open("../%s/build_lib" % os_dir)
  build_vars = {
    'prefix': sys.prefix,
    'exec_prefix': sys.exec_prefix,
    'python_inc_dir': distutils.sysconfig.get_python_inc(),
    'python_lib_dir': distutils.sysconfig.get_python_lib()
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
  if (len(argv) != 2 or
      argv[1].lower() not in ["mac", "windows", "linux"]):
    print "Usage: build_and_test.py <OS_target>"
    print
    print "<OS_target> can be Mac, Windows, or Linux"
    return

  os.chdir(os.path.dirname(os.path.realpath(__file__)))
  os_dir = "%s%s" % (argv[1][0:1].upper(), argv[1][1:].lower())
  print "Build and test file unpacker library for %s Portable Server." % os_dir

  if BuildLibrary(os_dir, argv[1].lower()=="windows"):
    print "Library built."
    RunTests()
  else:
    sys.exit(1)


if __name__ == "__main__":
  main(sys.argv)
