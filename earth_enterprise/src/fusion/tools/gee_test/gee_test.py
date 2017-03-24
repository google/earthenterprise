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

"""Install Fusion and Earth Server, build tutorial databases, and test."""

import getpass
import os
import sys
import time

from tests.common import utils

FUSION_SILENT_INSTALL_TEMPLATE = "installfusion.properties"
SERVER_SILENT_INSTALL_TEMPLATE = "installserver.properties"
TMP_DIR = "/tmp"

BUILD_CHECK_WAIT_IN_SECS = 10
WAIT_FOR_SERVER_STOP = 5
WAIT_FOR_SERVER_START = 5
MIN_DISK_SPACE = 2000  # ~2GB


def CheckEnvironment(no_tests):
  """Make sure that we are not root and that we have prodaccess."""
  if utils.DiskSpace("/tmp") < MIN_DISK_SPACE:
    print "Insufficient disk space."
    exit(0)
  if getpass.getuser() == "root":
    print "Please do not run as root."
    print "In order to run tests, you must use your own prodaccess."
    exit(0)


def StopFusion():
  """Make sure Fusion isn't running."""
  utils.ExecuteCmd("sudo /etc/init.d/gefusion stop")


def StopServer():
  """Make sure that Earth Server isn't running."""
  utils.ExecuteCmd("sudo /etc/init.d/geserver stop")


def StartFusion():
  """Make sure Fusion is running."""
  utils.ExecuteCmd("sudo /etc/init.d/gefusion start")


def StartServer():
  """Make sure that Earth Server is running."""
  utils.ExecuteCmd("sudo /etc/init.d/geserver start")


def CheckRunningProcesses():
  """Look for processes that will prevent installer from running."""
  out = utils.ExecuteCmd("ps -ef")
  good = True
  check = ["gesystemmanager",
           "geresourceprovider",
           "gehttpd",
           "postgres"]
  for line in out.split("\n"):
    for target in check:
      if target in line and "grep" not in line:
        print "Need to kill:", line
        good = False
  if not good:
    exit(0)


def UnpackTarball(path):
  """Unpack installer from tarball."""
  if not os.path.exists(path):
    print "Unable to find %s." % path
    return False
  if not path.endswith("tar.gz"):
    print "Expecting %s to be installer tarball ending in .tar.gz" % path
    return False
  utils.ChDir(TMP_DIR)
  utils.ExecuteCmd("tar -xzf %s" % path)
  return True


def Clean(force=False):
  """Clean gevol containing asset and publish roots."""
  if not force:
    answer = raw_input("Are you sure you want to wipe out your assets? [y/n] ")
    if answer != "y":
      print "Skipping clean ..."
      return
  utils.ExecuteCmd("sudo rm -rf /usr/local/google/gevol_test/assets/*")
  utils.ExecuteCmd("sudo rm -rf /usr/local/google/gevol_test/published_dbs/*")
  utils.ExecuteCmd("sudo rm -rf /opt/google/gehttpd")
  utils.ExecuteCmd("sudo rm -rf /var/opt/google/pgsql")
  utils.ExecuteCmd("sudo rm -rf /var/opt/google/fusion-backups")
  utils.ExecuteCmd("sudo rm -rf ~/.fusion")


def Uninstall():
  """Uninstall earlier versions of GEE."""
  found = False
  for uninstall_script in os.listdir("/opt/google"):
    if uninstall_script.startswith("Uninstall_Google_Earth_Enterprise_Fusion"):
      utils.ExecuteCmd("sudo /opt/google/%s" % uninstall_script)
      found = True
  if not found:
    print "No uninstaller was found."


def SetUpPropertiesFile(installer_dir, template, replace):
  """Create a properties file from the template."""
  utils.ChDir(installer_dir)
  content = utils.GetFileWithReplace(template, replace)
  fp = open("%s/installer.properties" % installer_dir, "w")
  fp.write(content)
  fp.close()
  utils.ExecuteCmd("chmod 755 installer.properties")


def InstallFusion(installer_dir, template):
  """Install Fusion using silent installer."""
  replace = {}
  SetUpPropertiesFile(installer_dir, template, replace)
  # Was not able to get it to work using a relative path to the properties file.
  utils.ExecuteCmd("sudo ./InstallGEFusion.sh %s/installer.properties"
                   % installer_dir)
  print "Fusion installed."


def InstallServer(installer_dir, template):
  """Install Earth Server using silent installer."""
  replace = {}
  SetUpPropertiesFile(installer_dir, template, replace)
  # Was not able to get it to work using a relative path to the properties file.
  utils.ExecuteCmd("sudo ./InstallGEServer.sh %s/installer.properties"
                   % installer_dir)
  print "Earth Server installed."


def BuildTutorial(base_dir):
  """Build tutorial databases."""
  utils.ChDir(base_dir)
  utils.ExecuteCmd("env PATH=\"$PATH:/opt/google/bin\" ./run_tutorial.sh")


def WaitForTutorialBuilds():
  """Wait until Tutorial databases are built."""
  dbs = ("SFDb_3d", "SFDb_3d_TM", "SF_2d_Merc")
  succeeded = {}
  for db in dbs:
    succeeded[db] = False
  done = False
  while not done:
    done = True
    for db in dbs:
      if not succeeded[db]:
        result = utils.ExecuteCmd(
            "sudo /opt/google/bin/gequery Databases/%s --status" %  db)
        if "Succeeded" not in result:
          done = False
          print "Waiting for %s." % db
        else:
          print "*** %s built! ***" % db
          succeeded[db] = True
    if not done:
      time.sleep(BUILD_CHECK_WAIT_IN_SECS)


def PushAndPublishTutorialDbs():
  """Publish the tutorial databases."""
  dbs = ("SFDb_3d", "SFDb_3d_TM", "SF_2d_Merc")
  # TODO: Be smarter about things that are already pushed and
  #  published. Right now, publishing in this way with unpublish if there is
  #  something already using the target.
  for db in dbs:
    result = utils.ExecuteCmd(
        "sudo /opt/google/bin/gequery Databases/%s --outfiles" % db)
    db_path = ""
    for line in result.split("\n"):
      if line.endswith("/gedb"):
        db_path = line
        break
      elif line.endswith("/mapdb"):
        db_path = line
        break
    if not db_path:
      raise Exception("Unable to find built database %s." % db)
    print "push and publish", db_path
    utils.ExecuteCmd("sudo /opt/google/bin/geserveradmin --adddb %s" % db_path)
    utils.ExecuteCmd("sudo /opt/google/bin/geserveradmin --pushdb %s" % db_path)
    # TODO: Use web api.
    utils.ExecuteCmd(
        "sudo /opt/google/bin/geserveradmin --publishdb %s --targetpath %s" %
        (db_path, db))


def RunTests(name):
  """Run all .py files in the current directory as tests."""
  num_tests = 0
  successes = 0
  for test in os.listdir("."):
    if not test.startswith("__") and test.endswith(".py"):
      num_tests += 1
      result = utils.ExecuteCmd("./%s" % test)
      lines = result.split("\n")
      num_lines = len(lines)
      for i in xrange(num_lines):
        if lines[num_lines - 1 - i].strip():
          if "SUCCESS" in lines[num_lines - 1 - i]:
            successes += 1
          print result
          break
  print "***** Testing: %s *****" % name
  print "%d of %d tests passed.\n" % (successes, num_tests)


def RunTestSets(base_dir):
  """Extract and run test sets."""
  print "** For default test sets, please run:"
  print "fileutil cp /cns/ih-d/home/gee/test_sets/*.tgz test_sets/"
  utils.ChDir("%s/test_sets" % base_dir)
  for test_set_tarball in os.listdir("."):
    if test_set_tarball.endswith(".tgz"):
      # Extract tests
      utils.ExecuteCmd("tar -xzf %s" % test_set_tarball)
      utils.ChDir("test_set")
      # Run tests
      RunTests(test_set_tarball)
      utils.ChDir("..")
      # Clean up
      utils.ExecuteCmd("rm -rf test_set")


def RunLocalTests(base_dir):
  """Runs tests in the test directory."""
  utils.ChDir(base_dir)
  utils.ChDir("%s/tests" % base_dir)
  RunTests("local")


def Flag(flag, argv):
  return "-%s" % flag in argv or "--%s" % flag in argv


def Usage(cmd):
  print (" Usage: %s [-uninstall] [-nofusion] [-noserver] [-clean] "
         "<installer_tarball>") % cmd
  print ("... or: %s [-uninstall] [-nofusion] [-noserver] [-clean] "
         "<installer_directory>") % cmd
  print
  print " Flags:"
  print ("       clean      (optional) - remove Fusion and Earth Server "
         "directories and files.")
  print "                               ** WARNING ** hard deletes all files."
  print "       uninstall  (optional) - run uninstaller if present."
  print "       nofusion   (optional) - skip Fusion install."
  print "       noserver   (optional) - skip Earth Server install."
  print "       notutorial (optional) - skip building tutorial databases."
  print ("       nopublish  (optional) - skip pushing and publishing tutorial "
         "databases.")
  print "       notests    (optional) - skip running tests."
  print "       force      (optional) - do not ask for confirmation of clean."
  exit(0)


def main(argv):
  utils.ClearLog()
  fusion_template = os.path.join(utils.BaseDir(),
                                 FUSION_SILENT_INSTALL_TEMPLATE)
  server_template = os.path.join(utils.BaseDir(),
                                 SERVER_SILENT_INSTALL_TEMPLATE)
  if len(argv) < 2:
    Usage(argv[0])
  if argv[-1][0] == "-":
    Usage(argv[0])
  CheckEnvironment(Flag("notests", argv))

  path = argv[-1]
  if path[0] != "/":
    path = "%s/%s" % (utils.BaseDir(), path)
  if not UnpackTarball(argv[-1]):
    print "Assuming tarball is already unpacked."
    installer_dir = path
  else:
    # Remove ".tar.gz" from tarball name.
    installer_dir = path[:-7]
  print "Installer directory:", installer_dir

  # Prepare for installs
  StopFusion()
  StopServer()
  time.sleep(WAIT_FOR_SERVER_STOP)
  CheckRunningProcesses()

  # Do installations.
  if Flag("clean", argv):
    if Flag("force", argv):
      Clean(True)
    else:
      Clean()
  if Flag("uninstall", argv):
    Uninstall()
  else:
    print "Skipping uninstall."
  if not Flag("nofusion", argv):
    InstallFusion(installer_dir, fusion_template)
    print "sudo /etc/init.d/gefusion start"
  else:
    print "Skipping Fusion install."
  if not Flag("noserver", argv):
    InstallServer(installer_dir, server_template)
    print "sudo /etc/init.d/geserver start"
  else:
    print "Skipping Earth Server install."

  # Start installed versions of Fusion and Earth Server.
  StartFusion()
  StartServer()
  time.sleep(WAIT_FOR_SERVER_START)

  # Build tutorial.
  if not Flag("notutorial", argv):
    BuildTutorial(utils.BaseDir())
  else:
    print "Skipping building tutorial databases."

  # Wait for build, then push and publish tutorial dbs.
  if not Flag("nopublish", argv):
    WaitForTutorialBuilds()
    PushAndPublishTutorialDbs()
  else:
    print "Skipping pushing and publishing of tutorial databases."

  # Run tests.
  if not Flag("notests", argv):
    RunLocalTests(utils.BaseDir())
    RunTestSets(utils.BaseDir())
  else:
    print "Skipping tests."

  # Do called for tests.

if __name__ == "__main__":
  main(sys.argv)
