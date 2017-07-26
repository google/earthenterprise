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

import getpass
import os
import socket
import sys
import time

from tests.common import utils


HOSTNAME = socket.getfqdn()
FUSION_SILENT_INSTALL_TEMPLATE = "installfusion.properties"
SERVER_SILENT_INSTALL_TEMPLATE = "installserver.properties"
TMP_DIR = "/tmp"

BUILD_CHECK_WAIT_IN_SECS = 10
WAIT_FOR_SERVER_STOP = 5
WAIT_FOR_SERVER_START = 5
MIN_DISK_SPACE = 2000  # ~2GB

TUTORIAL_DBS = ("SFDb_3d", "SFDb_3d_TM", "SF_2d_Merc")

GEVOL_DIR = "/usr/local/google/gevol_test"
PUBLISHED_DBS_STREAM_SPACE_DIR = os.path.join(
    GEVOL_DIR,
    "published_dbs/stream_space")


def CheckEnvironment(no_tests):
  """Make sure that we are not root and that we have prodaccess."""
  print "Hostname: ", HOSTNAME
  if utils.DiskSpace("/tmp") < MIN_DISK_SPACE:
    print "Insufficient disk space."
    exit(0)
  if getpass.getuser() == "root":
    print "Please do not run as root."
    print "In order to run tests, you must use your own prodaccess."
    exit(0)
  if not no_tests:
    prodcertstatus = utils.ExecuteCmd("prodcertstatus")
    print prodcertstatus
    if "No valid" in prodcertstatus:
      utils.ExecuteCmd("prodaccess --nog")


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

  gevol_dir = GEVOL_DIR
  utils.ExecuteCmd("sudo rm -rf %s/*" % gevol_dir)
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


def WaitForTutorialBuilds(dbs):
  """Wait until Tutorial databases are built.

  Args:
    dbs: a list of database names.
  """
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


def GetGedbPath(db_name):
  """Gets gedb/mapdb path for specified database.

  Args:
    db_name: a database name.

  Returns:
    a gedb/mapdb path.
  """
  result = utils.ExecuteCmd(
      "/opt/google/bin/gequery Databases/%s --outfiles" % db_name)
  db_path = ""
  for line in result.split("\n"):
    if line.endswith("/gedb") or line.endswith("/mapdb"):
      db_path = line
      break

  return db_path


def GetGedbPathForDbs(dbs):
  """Gets gedb/mapdb path for list of databases.

  Args:
    dbs: a list of database names.

  Returns:
    a list of gedb/mapdb paths.
  """
  gedb_paths = []
  for db_name in dbs:
    gedb_paths.append(GetGedbPath(db_name))

  return gedb_paths


def GetPublishGedbPathForDbs(dbs):
  """Gets publish gedb/mapdb path for list of databases.

  Args:
    dbs: a list of database names/paths.

  Returns:
    a list of publish gedb/mapdb paths.
  """
  publish_gedb_paths = []
  publish_dbs_base_path = os.path.join(PUBLISHED_DBS_STREAM_SPACE_DIR, HOSTNAME)
  for db_name in dbs:
    if db_name and os.path.isabs(db_name):
      gedb_path = db_name
    else:
      gedb_path = GetGedbPath(db_name)

    publish_gedb_paths.append("{}{}".format(publish_dbs_base_path, gedb_path))

  return publish_gedb_paths


def PushAndPublishDbs(dbs):
  """Push and publish list of databases.

  Args:
    dbs: list of databases.

  Raises:
    Exception: when unable to find database.
  """
  # TODO: Be smarter about things that are already pushed and
  #  published. Right now, publishing in this way with unpublish if there is
  #  something already using the target.
  for db in dbs:
    db_path = GetGedbPath(db)
    if not db_path:
      raise Exception("Unable to find built database %s." % db)

    print "push and publish: ", db_path
    utils.ExecuteCmd("/opt/google/bin/geserveradmin --adddb %s" % db_path)
    utils.ExecuteCmd("/opt/google/bin/geserveradmin --pushdb %s" % db_path)
    # TODO: Use web api.
    utils.ExecuteCmd(
        "/opt/google/bin/geserveradmin --publishdb %s --targetpath %s" %
        (db_path, db))


def RunGeindexcheck(db_name):
  """Runs geindexcheck for specified database.

  Args:
    db_name: database name/path.
  """

  if db_name and os.path.isabs(db_name):
    db_path = os.path.normpath(db_name)
  else:
    db_path = os.path.join("Databases", db_name)

  result = utils.ExecuteCmd(
      "/opt/google/bin/geindexcheck --mode all --database %s" % db_path,
      do_log=False, err2out=True)

  if "Fusion Fatal:" in result:
    print "geindexcheck FAILED for database: ", db_path
    print "Result: ", result
  else:
    print "SUCCESS"


def RunGeindexcheckForDbs(dbs):
  """Runs geindexcheck for list of databases.

  Args:
    dbs: list of database names/paths.
  """
  for db_name in dbs:
    RunGeindexcheck(db_name)


def GetTests(base_dir):
  """Get tests from cns and unpack them."""
  utils.ChDir(base_dir)
  test_sets = [
      "/cns/ih-d/home/bpeterson/gee_tests/gold_tests.tgz"
      ]
  for test_set in test_sets:
    utils.ExecuteCmd("fileutil cp %s test_set.tgz" % test_set)
    utils.ExecuteCmd("tar -xzf test_set.tgz")
    for path in os.listdir("test_set"):
      utils.ExecuteCmd("cp -r test_set/%s tests/" % path)
    utils.ExecuteCmd("rm test_set.tgz")
    utils.ExecuteCmd("rm -rf test_set")
    print "installed test set %s" % test_set


def RunTests(base_dir):
  """Runs tests in the test directory."""
  utils.ChDir(base_dir)
  num_tests = 0
  successes = 0
  for test in os.listdir("tests"):
    utils.ChDir("%s/tests" % base_dir)
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
  print "*****"
  print "%d of %d tests passed." % (successes, num_tests)


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
  print ("       nogeindexcheck (optional) - skip geindexcheck running for "
         "tutorial databases.")
  print ("       nopublish  (optional) - skip pushing and publishing tutorial "
         "databases.")
  print "       notests    (optional) - skip running tests."
  print "       nogettests (optional) - skip getting tests from cns."
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

  dbs = TUTORIAL_DBS

  # Build tutorial.
  if not Flag("notutorial", argv):
    BuildTutorial(utils.BaseDir())
  else:
    print "Skipping building tutorial databases."

  if not Flag("nogeindexcheck", argv):
    WaitForTutorialBuilds(dbs)
    RunGeindexcheckForDbs(dbs)
  else:
    print "Skipping geindexcheck for tutorial databases"

  # Wait for build, then push and publish tutorial dbs.
  if not Flag("nopublish", argv):
    WaitForTutorialBuilds(dbs)
    PushAndPublishDbs(dbs)

    print "Run geindexcheck for pushed/published DBs:"
    publish_gedb_paths = GetPublishGedbPathForDbs(dbs)
    RunGeindexcheckForDbs(publish_gedb_paths)
  else:
    print "Skipping pushing and publishing of tutorial databases."

  # Run tests.
  if not Flag("notests", argv):
    if not Flag("nogettests", argv):
      GetTests(utils.BaseDir())
    else:
      print "Skipping updating of tests from cns."

    RunTests(utils.BaseDir())
  else:
    print "Skipping tests."

  # Do called for tests.

if __name__ == "__main__":
  main(sys.argv)
