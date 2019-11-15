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

"""A wrapper for cgi.FieldStorage object to sanitize submitted form values."""

import logging
import os
import posixpath
import re
import urllib

import errors
import utils

# It is used for debugging.
# logging.getLogger().setLevel(level=logging.DEBUG)


class IllegalCharacterError(errors.Error):
  """Thrown if illegal character found in user supplied argument."""
  pass


class FormWrap(object):
  """Class wrapping cgi.FieldStorage object.

  Provides accessors sanitizing different type of submitted form values.
  """

  FORBIDDEN_CHARACTERS_IN_PATH = [">", "<", "|", "`", "\"", "'", ";", " "]

  def __init__(self, form, do_sanitize=True):
    self._form = form
    self._do_sanitize = do_sanitize
    if not self._do_sanitize:
      logging.warning("FormWrap - NO SANITIZING")

    # The regular expression to match only alphanumeric characters, dashes,
    # underscores, and periods.
    self._pattern = re.compile(r"[a-zA-Z0-9\-\_\.]*")

  def keys(self):
    """Gets list of keys (form arguments).

    Returns:
      list of arguments in form.
    """
    return self._form.keys()

  def getvalue_original(self, key, default_val=None):
    """Gets original value - no sanitizing.

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument.
    """
    logging.debug("getvalue_original() - key: %s", key)
    return self._getvalue_raw(key, default_val)

  def getvalue(self, key, default_val=None):
    """Gets value with sanitizing for general data types.

    The alphanumeric characters, dashes, underscores, and periods are only
    allowed.
    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument when key exists and value have passed
      a sanitizing procedure, otherwise - default_val.
    """
    logging.debug("getvalue() - key: %s", key)

    if not self._do_sanitize:
      return self._getvalue_raw(key, default_val)

    val = self._getvalue_sanitized(key, default_val)
    logging.debug("getvalue() - val: %s", val)
    return val

  def getvalue_escaped(self, key, default_val=None):
    """Gets value with special characters HTML-escaped.

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      html-escaped value for requested argument.
    """
    logging.debug("getvalue_escaped() - key: %s", key)

    val = self._getvalue_raw(key, default_val)
    if not val:
      return val

    val = utils.HtmlEscape(val)
    logging.debug("getvalue_escaped() - val: %s", val)
    return val

  # TODO: consider to use urllib.quote()/unquote() for path
  # sanitizing. NOTE: In Client we do escape source, polygon, etc.
  def _sanitize_path(self, path):
    """Sanitizes a path.

    Normalizes path (posixpath), replaces spaces with underscore and checks for
    illegal characters - special characters are not allowed in path.

    Args:
      path: a path to be sanitized.
    Returns:
      sanitized path.
    Raises:
      IllegalCharacterError: have found an illegal character.
    """
    if not path:
      return path

    # TODO: we are unescaping value here since it is escaped
    # in Client! It may be not safe since FormWrap is a generic class.

    # Un-escape first.
    path_res = urllib.unquote(path)

    # Normalize path.
    path_res = self._normalize_path(path_res)

    # Check for illegal characters.
    if not self._validate_path(path_res):
      raise IllegalCharacterError(
          "ERROR: found illegal characters in path: %s" % path_res)

    return path_res

  def _normalize_path(self, path):
    """Normalizes path.

    Removes dot segments, replaces spaces with underscore.
    Args:
      path: a source path.
    Returns:
      a normalized path.
    """
    # Normalize path.
    path_res = posixpath.normpath(path)

    # Remove dot segments at the beginning.
    path_res = path_res.lstrip(".")

    # Note: normpath() for empty string returns '.'.
    if not path_res:
      return path_res

    # Normalize path one more time to properly handle relative paths like this:
    # normpath('b/c/../../../../../d/e/f/../g') -> '../../../d/e/g'
    # '../../../d/e/g'.lstrip('.') -> '/../../d/e/g'
    # normpath('/../../d/e/g') -> '/d/e/g'
    path_res = posixpath.normpath(path_res)

    # Replace spaces with underscore.
    path_res.replace(" ", "_")

    # TODO: consider to add ending '/' for URL paths only!?
    # Adds ending "/" in the result path if the source path has it.
    if path_res:
      if path[-1] == "/" and path_res[-1] != "/":
        path_res += "/"

    return path_res

  def _validate_path(self, path):
    """Checks whether a path is valid.

    Checks a path for special characters.
    Args:
      path: a path to be validated.
    Returns:
      whether a path is valid.
    """
    for forbidden_character in FormWrap.FORBIDDEN_CHARACTERS_IN_PATH:
      if forbidden_character in path:
        return False

    return True

  def _sanitize_url(self, url):
    """Santizes an URL.

    Args:
      url: an URL to be sanitized.
    Returns:
      a sanitized URL.
    Raises:
      InvalidValueError: the url is invalid.
    """
    server, path = utils.GetServerAndPathFromUrl(url)
    if path:
      path = self._sanitize_path(path)

    return "{0}{1}".format(server, path)

  def getvalue_url(self, key, default_val=None):
    """Gets an URL value (absolute or relative).

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument.
    """
    logging.debug("getvalue_url() - key: %s", key)
    val = self._getvalue_raw(key, default_val)

    if not val:
      return val

    if self._do_sanitize:
      val = self._sanitize_url(val)

    logging.debug("getvalue_url() - key: %s, val: %s", key, val)
    return val

  def getvalue_path(self, key, default_val=None):
    """Gets path value (filesystem path).

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument.
    """
    logging.debug("getvalue_path() - key: %s", key)
    val = self._getvalue_raw(key, default_val)

    if not val:
      return val

    if self._do_sanitize:
      val = self._sanitize_path(val)

    logging.debug("getvalue_path() - key: %s, val: %s", key, val)
    return val

  def getvalue_filename(self, key, default_val=None):
    """Gets filename value.

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument.
    Raises:
      IllegalCharacterError: have found an illegal character.
      InvalidValueError: the value is rather path than filename.
    """
    logging.debug("getvalue_filename() - key: %s", key)
    val = self._getvalue_raw(key, default_val)

    if not val:
      return val

    if self._do_sanitize:
      val = self._sanitize_path(val)

    # Additionally check if value is a filename, not a path.
    if os.path.basename(val) != val:
      raise errors.InvalidValueError(
          "Invalid value for argument %s: %s."
          " Should be a filename, not a path." % (key, val))

    logging.debug("getvalue_filename() - key: %s, val: %s", key, val)
    return val

  def getvalue_kml(self, key, default_val=None):
    """Gets kml/xml-string value.

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument.
    """
    logging.debug("getvalue_kml() - key: %s", key)
    val = self._getvalue_raw(key, default_val)
    # TODO: sanitizing for xml/kml.
    logging.debug("getvalue_kml() - val: %s", val)
    return val

  def getvalue_json(self, key, default_val=None):
    """Gets json-string value.

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument.
    """
    logging.debug("getvalue_json() - key: %s", key)
    val = self._getvalue_raw(key, default_val)
    # TODO: sanitizing for json.
    logging.debug("getvalue_json() - val: %s", val)
    return val

  def _getvalue_raw(self, key, default_val=None):
    """Gets value without sanitizing.

    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      value for requested argument.
      Note: the return value is unescaped (unquoted).
    """
    logging.debug("_getvalue_raw() - key: %s", key)
    val = self._form.getvalue(key, default_val)
    logging.debug("_getvalue_raw() - val: %s", val)
    return val

  def _getvalue_sanitized(self, key, default_val=None):
    """Gets value with sanitizing.

    The alphanumeric characters, dashes, underscores, and periods are only
    allowed.
    Args:
      key: requested argument.
      default_val: a default value to return if requested argument is not
                   present.
    Returns:
      a value for requested argument when key exists and the value have passed
      a sanitizing procedure, otherwise - default_val.
    """
    logging.debug("_getvalue_sanitized() - key: %s", key)
    val = self._form.getvalue(key, default_val)
    logging.debug("_getvalue_sanitized() - val: %s", val)
    if not val:
      return val

    if isinstance(val, list):
      result = []
      for item in val:
        if isinstance(item, list):
          logging.error(
              "A list item for argument '%s' of form has been dropped", key)
          continue
        match = self._pattern.match(item)
        result.append(
            match.group(0) if (
                match and (len(match.group(0)) == len(item))) else default_val)

      return result
    else:
      match = self._pattern.match(val)
      return (match.group(0) if (
          match and (len(match.group(0)) == len(val))) else default_val)


def main():
  pass


if __name__ == "__main__":
  main()
