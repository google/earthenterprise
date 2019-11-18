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

"""The serve_utils module.

Module contains serving utils.
"""

import datetime
import logging
import os
import re
import shutil

from common import exceptions
from common import glc_unpacker
from serve import basic_types
from serve import constants
from serve import kh_constants

# Globals used for determining Glx details.
TIMESTAMP_GMT_REGEX = r"(\d\d\d\d-\d\d-\d\d \d+:\d\d:\d\d)\s+GMT[\r\n]"
TIMESTAMP_REGEX = r"(\d\d\d\d-\d\d-\d\d \d+:\d\d:\d\d)[\r\n]"

DATE_TIME_ISO_FORMAT = "%Y-%m-%d %H:%M:%S"
DATE_ISO_FORMAT = "%Y-%m-%d"

NO_DESCRIPTION = "No description given."


def DatetimeToIsoFormat(timestamp):
  """Return a string representing the date and time in ISO 8601 format.

  Args:
    timestamp: datetime.datetime object with time zone info.
  Returns:
    a string representing the date and time in ISO 8601 with time zone info.
  """
  assert isinstance(timestamp, datetime.datetime)
  return timestamp.isoformat()


def DatetimeNoTzToIsoFormatUtc(timestamp):
  """Return a string representing the date and time in ISO 8601 format.

  Args:
    timestamp: datetime.datetime object without time zone info.
  Returns:
    a string representing the date and time in ISO 8601 with UTC time zone info.
  """
  assert isinstance(timestamp, datetime.datetime)
  timestamp = timestamp.replace(tzinfo=None)
  return "{0}+00:00".format(timestamp.isoformat())


def GetIso8601StrFromDateTimeStr(timestamp_str):
  """Gets ISO-8601 string with time zone info from date-time or date string.

  Args:
    timestamp_str: timestamp string in format "%Y-%m-%d %H:%M:%S" or "%Y-%m-%d".
  Returns:
    ISO-8601 formatted string with time zone info at UTC:
    "%Y-%m-%d %H:%M:%S+00:00 or None if fails to extract date.
  """
  timestamp_datetime = None
  try:
    timestamp_datetime = datetime.datetime.strptime(
        timestamp_str, DATE_TIME_ISO_FORMAT)
  except ValueError:
    try:
      timestamp_datetime = datetime.datetime.strptime(
          timestamp_str[:10], DATE_ISO_FORMAT)
    except ValueError as e:
      logging.warning(
          "Timestamp format is not valid. Error: %s", e)
  if timestamp_datetime:
    return DatetimeNoTzToIsoFormatUtc(timestamp_datetime)
  else:
    return None


def GlxDetails(db_info):
  """Sets timestamp, desc, size, is_3d, is_2d for portable database.

  Args:
    db_info: portable database object.
  Returns:
    updated portable object
  """

  unpacker_ = None

  # Build path for portable globe.
  db_info.path = os.path.normpath("{0}/{1}".format(
      constants.CUTTER_GLOBES_PATH, db_info.name))

  # Set projection of Portable Globe.
  if IsGlc(db_info.type):
    unpacker_ = GetUnpacker(db_info)
    db_info.is_3d = unpacker_.Is3d()
    db_info.is_2d = unpacker_.Is2d()
  else:
    db_info.is_3d = Is3d(db_info.type)
    db_info.is_2d = Is2d(db_info.type)

  # Get timestamp and description for unregistered portables.
  if not db_info.registered:
    db_info.size = os.path.getsize(db_info.path)

    # TODO: do not set value for description in backend in case of
    # there is no portable info and set display value in frontend.
    # Initialize to None in DbInfo.

    # Set default values in event of empty timestamp/description.
    db_info.timestamp = None
    db_info.description = NO_DESCRIPTION

    if not unpacker_:
      unpacker_ = GetUnpacker(db_info)

    info = unpacker_.Info()
    if info:
      # Search info for a description and timestamp.
      entries = re.split(r"[\r\n]*\d\d\d\d-\d\d-\d\d \d+:\d\d:\d\d[\r\n]+",
                         info)

      if len(entries) > 1 and entries[1][:19] == "Globe description: ":
        description = entries[1][19:]
        description = description.replace("\n", "")
        description = description.replace("\"", "'")
        db_info.description = description
      else:
        logging.warning("Unable to find description for %s.", db_info.name)

      match = re.search(TIMESTAMP_GMT_REGEX, info)
      timestamp_str = None
      if match:
        timestamp_str = match.group(1)
      else:
        match = re.search(TIMESTAMP_REGEX, info)
        if match:
          timestamp_str = match.group(1)

      if timestamp_str:
        db_info.timestamp = GetIso8601StrFromDateTimeStr(timestamp_str)
      else:
        logging.warning("Unable to find timestamp for %s.", db_info.name)
    else:
      logging.warning("Unable to find info for %s.", db_info.name)


def GetUnpacker(db_info):
  """Unpacks a portable using glc_unpacker and returns object."""
  reader_ = glc_unpacker.PortableGlcReader(db_info.path)
  return glc_unpacker.GlcUnpacker(reader_, IsGlc(db_info.type), False)


def IsFusionDb(db_type):
  """Returns whether db_type is Fusion DB type."""
  return db_type in [basic_types.DbType.TYPE_GE, basic_types.DbType.TYPE_MAP]


def IsPortable(db_type):
  """Returns whether db_type is Portable type."""
  return db_type in [basic_types.DbType.TYPE_GLB,
                     basic_types.DbType.TYPE_GLM,
                     basic_types.DbType.TYPE_GLC]


def Is2d(db_type):
  """Returns whether database/portable of specific db_type is 2D."""
  return db_type in [basic_types.DbType.TYPE_MAP,
                     basic_types.DbType.TYPE_GLM]


def Is3d(db_type):
  """Returns whether database/portable of specific db_type is 3D."""
  return db_type in [basic_types.DbType.TYPE_GE,
                     basic_types.DbType.TYPE_GLB]


def IsGlc(db_type):
  """Returns whether database/portable of specific db_type is GLC."""
  return db_type == basic_types.DbType.TYPE_GLC


def IsPortablePath(path):
  """Returns whether specified path is portable globe path.

  Args:
    path: portable globe path.
  Returns:
    whether specified path is portable globe path.
  """
  return os.path.splitext(path)[1] in basic_types.PORTABLE_EXTS_SET


def ExistsPortableInDir(path):
  """Returns whether specified directory contains portable globes.

  Args:
    path: directory path
  Returns:
    whether specified directory contains portable globes.
  """
  assert os.path.isdir(path)
  for file_path in os.listdir(path):
    if IsPortablePath(file_path):
      return True

  return False


def IdentifyPublishedDb(db_name):
  """Identifies type of published database.

  Args:
    db_name: database name that is actually an DB asset path.
  Returns:
    (db_path, db_type): database path, type tuple.
  Raises:
    exceptions.Error in case of short database name.
  """
  if not db_name:
    raise exceptions.Error(
        "Internal Error - database name is an empty string.")

  db_path = os.path.normpath(db_name)
  db_type = None

  if db_path[-5:] == "/gedb":
    db_type = basic_types.DbType.TYPE_GE
  elif db_path[-6:] == "/mapdb":
    db_type = basic_types.DbType.TYPE_MAP
  else:
    db_ext = db_path[-4:]
    if db_ext == ".glb":
      db_type = basic_types.DbType.TYPE_GLB
    elif db_ext == ".glm":
      db_type = basic_types.DbType.TYPE_GLM
    elif db_ext == ".glc":
      db_type = basic_types.DbType.TYPE_GLC
    else:
      raise exceptions.Error(
          "Internal Error - Invalid database type for %s." % db_path)

  # Check for short database name for Fusion DB.
  if (db_type == basic_types.DbType.TYPE_GE or
      db_type == basic_types.DbType.TYPE_MAP):
    if db_name[0] != "/":
      raise exceptions.Error(
          "Internal Error - The short database name (%s) cannot be used." %
          db_name)

  return (db_path, db_type)


def NormalizeTargetPath(target_path):
  """Normalizes target path.

  Args:
    target_path: target path to normalize.
  Returns:
    normalized target path or None.
  """
  norm_target_path = None
  if target_path:
    norm_target_path = os.path.normpath(target_path)
    # Strip leading and ending spaces and slashes.
    norm_target_path = norm_target_path.strip("/ ")
    # TODO: do not add leading slash?
    if norm_target_path:
      norm_target_path = "/{0}".format(norm_target_path)

  return norm_target_path


def CalcDatabaseAttributes(db_info):
  """Calculates database attributes.

  Calculates database attributes (type, is_2d, is_3d, is_mercator)
  based on path and name.
  Note/TODO:
    is_2d, is_3d is undefined for *.glc
  Args:
    db_info: database info.
  """
  assert db_info.path
  assert db_info.name

  (db_info.path,
   db_info.type) = IdentifyPublishedDb(db_info.path)
  if IsFusionDb(db_info.type):
    if db_info.type == basic_types.DbType.TYPE_MAP:
      db_info.is_mercator = IsMercatorMap(db_info.path)

    db_info.name = GetFusionDbInfoName(
        db_info.name, db_info.type, db_info.is_mercator)
    db_info.is_3d = Is3d(db_info.type)
    db_info.is_2d = Is2d(db_info.type)
  else:
    assert IsPortable(db_info.type)
    db_info.name = db_info.path
    GlxDetails(db_info)


def IsMercatorMap(db_path):
  """Checks whether Fusion database is a Mercator map based on DB path.

  Checks by suffix in the path: kmdatabase vs kmmdatabase.
  examples of path:
  /.../Databases/NY.kmdatabase/mapdb.kda/ver001/mapdb/ - Flat.
  /.../Databases/NY.kmmdatabase/mapdb.kda/ver001/mapdb/ - Mercator.

  Args:
    db_path: Fusion database (map) path.
  Returns:
    whether Fusion database is a Mercator map.
  """
  if kh_constants.MERCATOR_MAP_DATABASE_PATTERN in db_path:
    return True
  else:
    assert kh_constants.MAP_DATABASE_PATTERN in db_path
    return False


def GetFusionDbInfoName(db_name, db_type, is_mercator):
  """Gets Fusion database name from database pretty name.

  Database pretty name is database asset root path or database path
  in case of disconnected publishing. Function extracts name and version
  from this path.
  e.g.
  "Databases/SF.kdatabase?version=207"
  db_info_name is "SF-v207".
  "/usr/local/google/tutorial/Databases/NY.kdatabase/gedb.kda/ver072/gedb/"
  db_info_name is "NY-v072".
  Args:
    db_name: database name that is database path.
    db_type: database type.
  Raises:
    PublishServeException.
  Returns:
    Fusion database friendly name in format "name-v001".
  """
  # Try to extract database name from db_pretty_name that is asset root DB
  # path, by default.
  # Getting name for gedb database.
  if db_type == basic_types.DbType.TYPE_GE:
    db_suffix = kh_constants.DATABASE_SUFFIX
  elif db_type == basic_types.DbType.TYPE_MAP:
    if is_mercator:
      db_suffix = kh_constants.MERCATOR_MAP_DATABASE_SUFFIX
    else:
      db_suffix = kh_constants.MAP_DATABASE_SUFFIX
  else:
    assert False, "Not valid DB type %s." % db_type

  # Look for suffix from the end of string to manage the case when database
  # name is equal to the suffix.
  db_name_eidx = db_name.rfind(db_suffix)
  if db_name_eidx == -1:
    # Just return db_name since it may be database alias specified by user.
    return db_name

  db_name_bidx = db_name.rfind("/", 0, db_name_eidx)
  if db_name_bidx != -1:
    # Extract Fusion database name from path.
    db_info_name = db_name[db_name_bidx + 1:db_name_eidx]
  else:
    # Just return db_name since it may be database alias specified by user.
    return db_name

  # Get Fusion DB version from db_pretty_name.
  ver_bidx = db_name.find("?version=")
  if ((ver_bidx != -1) and (ver_bidx + 9 < len(db_name))
      and db_name[ver_bidx + 9:].isdigit()):
    db_version = int(db_name[ver_bidx + 9:])
    db_info_name = "%s-v%03d" % (db_info_name, db_version)
  else:
    ver_bidx = db_name.find(kh_constants.DB_KEY_SUFFIX + "/ver")
    if ver_bidx == -1:
      # Just return db_name since it may be database alias specified by user.
      return db_name

    ver_bidx += len(kh_constants.DB_KEY_SUFFIX) + 4

    if ver_bidx >= len(db_name):
      # Just return db_name since it may be database alias specified by user.
      return db_name

    ver_eidx = db_name.find("/", ver_bidx)
    if (ver_eidx == -1) or (not db_name[ver_bidx:ver_eidx].isdigit()):
      # Just return db_name since it may be database alias specified by user.
      return db_name

    db_version = int(db_name[ver_bidx:ver_eidx])
    db_info_name = "%s-v%03d" % (db_info_name, db_version)

  return db_info_name


def LocalTransfer(src_path, dest_path, force_copy=False,
                  prefer_copy=False, allow_symlinks=True):
  """Transfers specified source file to specified destination path.

  Args:
    src_path: source file path.
    dest_path: destination file path.
    force_copy: whether to force a copy.
    prefer_copy: whether copying is preferred.
    allow_symlinks: whether symlink is allowed.
  Returns:
    whether transfer was successful.
  """
  try:
    # Ensure that source file exists and has read permission.
    # Have to check since it is allowed to create a link to a non-existing file.
    if not os.path.exists(src_path):
      raise exceptions.Error(
        "Source file has no read permission or does not exist: %s" % src_path)
    elif not os.access(src_path, os.R_OK):
      raise exceptions.Error("No read permission: %s" % src_path)

    # Ensure that the destination parent directory exists.
    if not os.path.exists(os.path.dirname(dest_path)):
      os.makedirs(os.path.dirname(dest_path))
    elif os.path.exists(dest_path):
      # Delete existing destination file.
      os.unlink(dest_path)

    # If force_copy is set to Y then just copy.
    # Note: force_copy can be set by user (parameter for
    # console tool). Use case is a disconnected publishing.
    if force_copy:
      shutil.copy2(src_path, dest_path)
      return True

    # force_copy are not set to Y, then try a hard-link
    # at first.
    # Not: os.link tries to create hard link, and if it fails,
    # it then creates a copy.
    try:
      os.link(src_path, dest_path)
      # Compare the inode numbers to verify whether we get a copy
      # or a hard link.
      if os.path.exists(dest_path) and os.path.samefile(src_path, dest_path):
        return True
    except OSError:
      pass

    # Hard-link failed.
    # Note: we set prefer_copy if src_path is a path in temp.
    # directory or file is going to be updated then (e.g. layers.js).
    if prefer_copy:
      # os.link() can make a copy, so we check for existence.
      if not os.path.exists(dest_path):
        shutil.copy2(src_path, dest_path)
      return True
    else:
      # if allow-symlinks is set to Y then try to create a symlink.
      if allow_symlinks:
        try:
          # os.link() can make a copy, so we check for existence.
          if os.path.exists(dest_path):
            os.unlink(dest_path)

          os.symlink(src_path, dest_path)
          return True
        except OSError:
          pass

      # Symlink is not allowed or failed. Copy is the last choice.
      # os.link() can make a copy so we check for existence.
      if not os.path.exists(dest_path):
        shutil.copy2(src_path, dest_path)
      return True
  except (OSError, IOError, shutil.Error, exceptions.Error) as e:
    logging.error("Local transfer failed.\n %s", e)
  except Exception as e:
    logging.error("Local transfer failed.\n Unknown error: %s", e)

  return False


def FileSizeMatched(file_path, file_size):
  """Matches real size of file with the file_size fetched from database.

  Args:
    file_path: file to be matched.
    file_size: size of file which we match with.

  Returns:
    whether files are matched.
  """
  length = os.path.getsize(file_path)
  logging.debug(
      "FileSizeMatched: file_path: %s file_size: %d file.length(): %d",
      file_path, file_size, length)
  if length == file_size:
    logging.debug("Matched!")
    return True
  logging.debug("Did not match.")
  return False


def main():
  pass


if __name__ == "__main__":
  main()
