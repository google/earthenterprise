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


"""Holds meta-information about the image formats we support."""

import collections


ImageSpec = collections.namedtuple(
    "ImageSpec", "content_type file_extension pil_format")

IMAGE_SPECS = {"jpg": ImageSpec("image/jpeg", "jpg", "JPEG"),
               "png": ImageSpec("image/png", "png", "PNG")
              }


def IsKnownFormat(fmt):
  """Checks if the format is supported.

  Args:
    fmt: Format of the image.
  Returns:
    boolean: If the format is supported.
  """
  for spec in IMAGE_SPECS.values():
    if spec.content_type == fmt:
      return True
  return False


def GetImageSpec(fmt):
  """Get the Imagespec.

  Args:
    fmt: Format of the image.
  Returns:
    image_spec: image spec.
  """
  for spec in IMAGE_SPECS.values():
    if spec.content_type == fmt:
      return spec

  return None


def FormatIsPng(fmt):
  """Checks if the format is of type png.

  Args:
    fmt: Format of the image.
  Returns:
    boolean: If the format is png or not.
  """
  for typ, spec in IMAGE_SPECS.iteritems():
    if spec.content_type == fmt:
      return typ == "png"
  return False


def main():
  is_format = IsKnownFormat("jpeg")
  print is_format

if __name__ == "__main__":
  main()
