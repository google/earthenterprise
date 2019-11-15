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

"""The AssetManager handler implementation module.

The AssestManager handler executes asset managing request.
"""

from common import exceptions
from serve import constants


class AssetManagerHandler(object):
  """Processes asset manager requests."""

  def __init__(self):
    """Initializes AssetManagerHandler."""
    pass

  def HandleListAssetsRequest(self, request, response):
    cmd = request.GetParameter(constants.CMD)
    raise exceptions.AssetManagerServeException(
        "Not supported command: {0}.".format(cmd))


def main():
  pass


if __name__ == "__main__":
  main()
