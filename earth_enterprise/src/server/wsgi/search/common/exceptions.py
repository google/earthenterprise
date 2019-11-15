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

"""Module for all exception's which search services may raise."""

from search.common import utils


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
      return utils.HtmlEscape(
          "{0}: {1}".format(error_prefix, str("\n".join(self.args))))
    else:
      return utils.HtmlEscape("Error: {0}".format(str("\n".join(self.args))))

  def __str__(self):
    return self.ToString("Error")


class BadQueryException(Error):
  """BadQueryException error."""

  def __str__(self):
    return self.ToString("BadQueryException")


# Places search service pool exception.
class PoolConnectionException(Error):
  """PoolConnectionException error."""

  def __str__(self):
    return self.ToString("PoolConnectionException")


def main():
  pass


if __name__ == "__main__":
  main()
