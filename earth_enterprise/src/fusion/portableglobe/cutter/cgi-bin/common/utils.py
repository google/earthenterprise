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


"""Utilities for support of portable globes."""

import json
import os
import shlex
import subprocess
import sys
import time
import urlparse
import xml.sax.saxutils as saxutils
import distutils.dir_util
import distutils.errors

import errors

BYTES_PER_MEGABYTE = 1024.0 * 1024.0
NAME_TEMPLATE = "%s_%s"


class OsCommandError(Exception):
  """Thrown if os command fails."""
  pass


# TODO: consider to use a lib like bleach that is specifically
# aimed at foiling XSS attacks.

# Additional characters that need to be escaped for HTML defined in a dictionary
# the character to its escape string.
# xml.sax.saxutils.escape() takes care of &, < and >.
_HTML_ESCAPE_TABLE = {
    '"': "&#34;",
    "'": "&#39;",
    "`": "&#96;",
    "|": "&#124;"
    }


def HtmlEscape(text):
  """Escapes a string for HTML.

  Args:
    text: source string that needs to be escaped for HTML.
  Returns:
    HTML escaped string.
  """
  if not text:
    return text

  return saxutils.escape(text, _HTML_ESCAPE_TABLE)


def FileSize(file_path):
  """Returns size of file in megabytes."""
  return os.path.getsize(file_path) / BYTES_PER_MEGABYTE


def SizeAsString(size):
  """Converts megabyte float to a string."""
  if size < 1000.0:
    return "%0.2fMB" % size
  size /= 1024.0
  if size < 1000.0:
    return "%0.2fGB" % size
  else:
    return "%0.2fTB" % (size / 1024.0)


def FileSizeAsString(file_path):
  """Returns size of file as a string."""
  return SizeAsString(FileSize(file_path))


def DirectorySize(directory):
  """Returns size of directory in megabytes."""
  directory_size = 0
  if os.path.isdir(directory):
    for (path, unused_dirs, files) in os.walk(directory):
      for file_name in files:
        file_path = os.path.join(path, file_name)
        directory_size += os.path.getsize(file_path)

  return directory_size / BYTES_PER_MEGABYTE


def DirectorySizeAsString(directory):
  """Returns size of directory as a string."""
  return SizeAsString(DirectorySize(directory))


def CreateDirectory(directory):
  """Create entire directory path."""
  if os.path.exists(directory):
    return
  try:
    os.makedirs(directory)
  except OSError:
    PrintAndLog("Raising error: Cannot create directory \'%s\'" % directory)
    raise


def CopyDirectory(source, destination, logger):
  """Copy from source to destination, which will be created if it does not exist."""
  cmd = "Copying %s to %s" % (source, destination)
  PrintAndLog(cmd, logger)
  try:
    distutils.dir_util.copy_tree(source, destination)
  except distutils.errors.DistutilsFileError:
    PrintAndLog("Raising error: Cannot copy to directory %s" % destination)
    raise


def DiskSpace(path):
  """Returns remaining disk space in Megabytes."""
  mount_info = os.statvfs(path)
  return mount_info.f_bsize * mount_info.f_bavail / BYTES_PER_MEGABYTE


def Uid():
  """Returns a uid for identifying a globe building sequence."""
  return "%d_%f" % (os.getpid(), time.time())


def GlobesToText(globes, template, sort_item, reverse=False, is_text=False):
  """Fills in globe template for each globe and returns as array of strings."""
  result = []
  # If it is text, sort the lower case version of the text.
  if is_text:
    items = sorted(globes.iteritems(),
                   key=lambda globe_pair: globe_pair[1][sort_item].lower(),
                   reverse=reverse)
  # If it is NOT text, use default less than comparison.
  else:
    items = sorted(globes.iteritems(),
                   key=lambda globe_pair: globe_pair[1][sort_item],
                   reverse=reverse)
  for [unused_key, globe] in iter(items):
    next_entry = template
    for [globe_term, globe_value] in globe.iteritems():
      replace_item = "[$%s]" % globe_term.upper()
      if globe_term == "globe" or globe_term == "info_loaded":
        pass
      elif globe_term == "size":
        next_entry = next_entry.replace(replace_item, SizeAsString(globe_value))
      else:
        next_entry = next_entry.replace(replace_item, globe_value)
    result.append(next_entry)
  return result


def GlobeNameReplaceParams(globe_name):
  """Returns a single replacement parameter for the globe name."""
  return {"[$GLOBE_NAME]": globe_name}


def ReplaceParams(text, replace_params):
  """Replace keys with values in the given text."""
  for (key, value) in replace_params.iteritems():
    text = text.replace(key, value)
  return text


def OutputFile(file_name, replace_params):
  """Outputs a file to standard out with the globe name replaced."""
  fp = open(file_name)
  text = fp.read()
  fp.close()
  print ReplaceParams(text, replace_params)


def CreateInfoFile(path, description):
  """Create globe info file."""
  content = "Portable Globe\n"
  content += GmTimeStamp()
  content += "\n%s" % TimeStamp()
  content += "Globe description: %s\n" % description
  CreateFile(path, content)


def CreateFile(path, content):
  """Create globe info file."""
  try:
    fp = open(path, "w")
    fp.write(content)
    fp.close()
  except IOError as error:
    print error
    sys.exit(1)


def TimeStamp():
  """Create timestamp based on local time."""
  return time.strftime("%Y-%m-%d %H:%M:%S\n", time.localtime())


def GmTimeStamp():
  """Create timestamp based on Greenwich Mean Time."""
  return time.strftime("%Y-%m-%d %H:%M:%S GMT\n", time.gmtime())


def ConvertToQtNode(level, col, row):
  """Converts col, row, and level to corresponding qtnode string."""
  qtnode = "0"
  half_ndim = 1 << (level - 1)
  for unused_ in xrange(level):
    if row >= half_ndim and col < half_ndim:
      qtnode += "0"
      row -= half_ndim
    elif row >= half_ndim and col >= half_ndim:
      qtnode += "1"
      row -= half_ndim
      col -= half_ndim
    elif row < half_ndim and col >= half_ndim:
      qtnode += "2"
      col -= half_ndim
    else:
      qtnode += "3"

    half_ndim >>= 1

  return qtnode


def JsBoolString(bool_value):
  """Write boolean value as javascript boolean."""
  if bool_value:
    return "true"
  else:
    return "false"


def WriteHeader(content_type="text/html"):
  """Output header for web page."""
  # Pick up one print from the Python print.
  print "Content-Type: %s\n" % content_type


def ExecuteCmd(os_cmd, logger, dry_run=False):
  """Execute os command and log results.

  Runs command, waits until it finishes, then analyses the return code, and
  reports either "SUCCESS" or "FAILED".
  Use if output of command is not desired, otherwise it should be redirected
  to a file or use RunCmd below.

  Args:
    os_cmd: Linux shell command to execute.
    logger: Logger responsible for outputting log messages.
    dry_run: Whether command should only be printed but not run.
  Throws:
    OsCommandError
  """
  PrintAndLog("Executing: %s" % os_cmd, logger)
  if dry_run:
    PrintAndLog("-- dry run --", logger)
    return
  try:
    if isinstance(os_cmd, str):
      os_cmd = shlex.split(os_cmd)
    p = subprocess.Popen(os_cmd, shell=False,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    err_data = p.communicate()[1]
    return_code = p.returncode
    if return_code != 0:
      PrintAndLog("Raising error: %s (return code %d)\n"
                  % (err_data, return_code), logger)
      raise OsCommandError()
    else:
      PrintAndLog("SUCCESS", logger, None)
  except Exception, e:
    PrintAndLog("FAILED: %s" % e.__str__(), logger)
    raise OsCommandError()


def ExecuteCmdInBackground(os_cmd, logger):
  """Execute os command in the background and log results.

  Runs command in the background and returns immediately without waiting for
  the execution to finish.
  Use if the command will take longer time to finish than request timeout.

  Args:
    os_cmd: Linux shell command to execute.
    logger: Logger responsible for outputting log messages.
  Throws:
    OsCommandError
  """
  PrintAndLog("Executing in background: %s" % os_cmd, logger)
  try:
    subprocess.Popen(os_cmd + " &", shell=True,
                     stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE)
  except Exception, e:
    PrintAndLog("FAILED: %s" % e.__str__(), logger)
    raise OsCommandError()


def RunCmd(os_cmd):
  """Execute os command and return list of results and errors.

  Runs command, waits until it finishes, then returns the output of execution
  (if succeeded) or error information (if failed).
  Use if output of command is needed.

  Args:
    os_cmd: Linux shell command to execute.
  Returns:
    Array of result lines.
  """
  try:
    if isinstance(os_cmd, str):
      os_cmd = shlex.split(os_cmd)
    p = subprocess.Popen(os_cmd, shell=False,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    # capture stdout/stderr into memory variables
    # NOTE: communicate() can only be called one time
    # per call of Popen as after the first call
    # stdout/stderr pipe handles are closed
    results, err_data = p.communicate()
    return_code = p.returncode
    if return_code != 0:
      results = "{0} (return code {1})".format(err_data, return_code)
      return ["", results]
    else:
      return results.split("\n")
  except Exception, e:
    # print "FAILURE: %s" % e.__str__()
    return ["", e.__str__()]


def PrintAndLog(msg, logger=None, prefix="\n"):
  if prefix:
    print "%s%s" % (prefix, msg)
  else:
    print msg

  if logger:
    logger.Log(msg)


def GetDbrootInfoJson(globe, name):
  """Get dbroot info as a json string.

  Args:
    globe: portable_globe object.
    name:  name of portable globe
  Returns:
    Dbroot info in Json formatted string.
  """
  dbroot_info = {"name": name,
                 "has_imagery": globe.HasImagery(),
                 "has_terrain": globe.HasTerrain(),
                 "is_proto_imagery": globe.IsProtoImagery(),
                }
  return json.dumps(dbroot_info)


def NormalizeTargetPath(target):
  """Normalizes the target path.

  Adds leading slash if needed, strips ending slashes.
  Args:
    target: The target path (fusion db publish point).
  Returns:
    Normalized target path.
  """
  if not target:
    return target

  target = target.strip()
  target = target.rstrip("/")

  if not target:
    return target

  if target[0] != "/":
    target = "/{0}".format(target)

  return target


def GetServerAndPathFromUrl(url):
  """Gets a server and a path from the url.

  Args:
    url: the URL.
  Returns:
    tuple (server, path). The server is 'scheme://host:port'.
    The path can be empty string.
  Raises:
    InvalidValueError: when the url is not valid.
  """
  server = ""
  path = ""
  url_obj = urlparse.urlparse(url)
  if url_obj.scheme and url_obj.netloc and url_obj.path:
    server = "{0}://{1}".format(url_obj.scheme, url_obj.netloc)
    path = url_obj.path
  elif url_obj.scheme and url_obj.netloc:
    server = "{0}://{1}".format(url_obj.scheme, url_obj.netloc)
  elif url_obj.path:
    path = url_obj.path
  else:
    raise errors.InvalidValueError("Invalid URL: %s" % url)

  return (server, path)


def IsProcessRunningForGlobe(tool_name, base_dir):
  """Checks whether specified job is running for portable.

  Checks if process is running by detecting it in the output returned by
  executing "ps -ef | grep base_dir".

  Args:
    tool_name: tool name to check if it is present in list of running
               processes.
    base_dir: base directory for corresponding portable.
  Returns:
    whether specified job is running.
  """
  ps_cmd = "ps -ef"
  grep_cmd = "grep \"%s\"" % base_dir
  ps_subprocess = subprocess.Popen(shlex.split(ps_cmd),
                                   shell=False,
                                   stdout=subprocess.PIPE)
  grep_subprocess = subprocess.Popen(shlex.split(grep_cmd),
                                     shell=False,
                                     stdin=ps_subprocess.stdout,
                                     stdout=subprocess.PIPE)
  ps_subprocess.stdout.close()  # Allow p1 to receive a SIGPIPE if p2 exits.
  procs = grep_subprocess.communicate()[0]

  if procs:
    procs = procs.split("/n")

    for proc in procs:
      if proc.find(tool_name) > 0:
        return True

  return False


class Log(object):
  """Simple logger class."""

  def __init__(self, log_file, enabled=True):
    self.log_file_ = log_file
    self.enabled_ = enabled

  def CheckLogFolder(self):
    return os.path.exists(os.path.dirname(self.log_file_))

  def Clear(self):
    """Clear the log file."""
    if not self.CheckLogFolder():
      return

    fp = open(self.log_file_, "w")
    fp.close()

  def Log(self, message):
    """Log message to cutter log."""
    if not self.enabled_ or not self.CheckLogFolder():
      return

    fp = open(self.log_file_, "a")
    fp.write("%s" % TimeStamp())
    fp.write("%s\n" % message)
    fp.close()
