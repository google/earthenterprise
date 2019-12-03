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

"""Generic exceptions and auxiliary functions to handle exception."""

import cgitb

import utils

def FormatException(exc_info):
  """Gets information from exception info tuple.

  Args:
    exc_info: exception info tuple (type, value, traceback)
  Returns:
    exception description in a list - wsgi application response format.
  """
  return [cgitb.handler(exc_info)]


class Error(Exception):
  """Generic error."""

  def ToString(self, error_prefix):
    """Builds error message string escaping it for HTML.

    Args:
      error_prefix: an error prefix.
    Returns:
      HTML escaped error message.
    """
    if error_prefix:
      return utils.HtmlEscape("{0}: {1}".format(
          error_prefix,
          str(self.args[0] if len(self.args) <= 1 else self.args)))
    else:
      return utils.HtmlEscape("Error: {0}".format(
          str(self.args[0] if len(self.args) <= 1 else self.args)))

  def __str__(self):
    return self.ToString("Error")


# Stream push serving exception.
class StreamPushServeException(Error):
  """StreamPushServe error."""

  def __str__(self):
    return self.ToString("StreamPushServe")


# Search push serving exception.
class SearchPushServeException(Error):
  """SearchPushServe error."""

  def __str__(self):
    return self.ToString("SearchPushServe")


# Search publish serving exception.
class SearchPublishServeException(Error):
  """SearchPublishServe error."""

  def __str__(self):
    return self.ToString("SearchPublishServe")


# Publish serving exceptions.
class PublishServeException(Error):
  """PublishServe error."""

  def __str__(self):
    return self.ToString("PublishServe")


# Fdb module serving exception.
class ModFdbServeException(PublishServeException):
  """ModFdbServe error."""

  def __str__(self):
    return self.ToString("ModFdbServe")


# Batch SQL manager exception.
class BatchSqlManagerException:
  """BatchSqlManager error."""

  def __str__(self):
    return self.ToString("BatchSqlManager")


# SearchSchemaTableUtility exception.
class SearchSchemaTableUtilException(Error):
  """SearchSchemaTableUtil error."""

  def __str__(self):
    return self.ToString("SearchSchemaTableUtil")


# SearchSchemaParser exception.
class SearchSchemaParserException(Error):
  """SearchSchemaParser error."""

  def __str__(self):
    return self.ToString("SearchSchemaParser")


# Snippets serving exception.
class SnippetsServeException(Error):
  """SnippetServe error."""

  def __str__(self):
    return self.ToString("SnippetsServe")


# GEE publisher
class StreamPublisherServletException(Error):
  """StreamPublisherServlet error."""

  def __str__(self):
    return self.ToString("StreamPublisherServlet")


# Fusion AssetManager serving exception.
class AssetManagerServeException(Error):
  """Fusion AssetManager serving error."""

  def __str__(self):
    return self.ToString("AssetManager")


def main():
  pass

if __name__ == "main":
  main()
