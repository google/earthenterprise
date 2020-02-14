#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
# Copyright 2019-2020 Open GEE Contributors
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


"""Misc. utilities.

...like logging, Assert, and a case-insensitive
dictionary accessor.
"""

import cgi
import logging
import sys
import traceback
import urllib

# Get logger
logger = logging.getLogger("wms_maps")


def GetValue(dictionary, key_no_case):
  """Gets a value from a case-insensitive key.

  Args:
      dictionary: dict of key -> value
      key_no_case: your case-insensitive key.

  Returns:
      Value of first case-insensitive match for key_no_case.

  """
  key = key_no_case.lower()

  if dictionary.has_key(key):
    return dictionary[key]

  return None


# For Python /objects/.
def DumpParms(params, msg=None):
  """Dumps the params into the log file.

  Args:
      params: params to be dumped.
      msg: what to log along with the params.
  """
  if isinstance(params, basestring):
    logger.debug(msg + " " + params)
  elif isinstance(params, list):
    logger.debug(msg + " ")
    for entry in params:
      logger.debug(str(entry))
  else:
    for key, value in params.iteritems():
      text = ((msg if msg is not None else "") + " " + "%s=%s"
              % (str(key), str(value)))
      logger.debug(text)


def Assert(hopefully_true, msg=None):
  """Bare Python 'assert' hangs the server when it hits.

  Args:
      hopefully_true: What you're asserting.
      msg: what to log if the assertion fails.
  """
  if not hopefully_true:
    prt_msg = msg if msg else ""
    print >> sys.stderr, "ASSERTION FAILED", prt_msg
    logger.debug("ASSERTION FAILED: " + prt_msg)
    # goes to stderr => Apache error_log
    traceback.print_stack(sys.stderr)


def GetParameters(environ):
  """Fetch the attributes like SERVICE,REQUEST,VERSION etc.

  Args:
   environ: A list which contains the WSGI environment information.
  Returns:
   parameters: All of the query parameters (SERVICE, FORMAT etc), with
     the additions of this-endpoint.
  """
  parameters = {}
  form = cgi.FieldStorage(fp=environ["wsgi.input"], environ=environ)

  for key in form.keys():
    parameters[key.lower()] = form.getvalue(key)

  # Examples for "this-endpoint" would be 'http://localhost/<target_path>/wms'
  # or 'http://servername/<target_path>/wms'
  (parameters["server-url"],
   parameters["this-endpoint"],
   parameters["proxy-endpoint"]) = GetServerURL(environ)

  if parameters["proxy-endpoint"] is None:
    del parameters["proxy-endpoint"]

  return parameters


def GetServerURL(environ):
  """Process server end point from "environ".

  Args:
    environ: A list which contains the WSGI environment information.
  Returns:
    (server_url, complete_url): server URL (shceme+host+port) and complete URL (
    server_url/path).
  """
  server_url = environ["wsgi.url_scheme"] + "://"

  if environ.get("HTTP_HOST"):
    server_url += environ["HTTP_HOST"]
  else:
    server_url += environ["SERVER_NAME"]

    if environ["wsgi.url_scheme"] == "https":
      if environ["SERVER_PORT"] != "443":
        server_url += ":" + environ["SERVER_PORT"]
    else:
      if environ["SERVER_PORT"] != "80":
        server_url += ":" + environ["SERVER_PORT"]

  complete_url = server_url

  complete_url += urllib.quote(environ.get("REDIRECT_URL", ""))
  complete_url += urllib.quote(environ.get("PATH_INFO", ""))

  if environ.get("HTTP_X_FORWARDED_HOST"):
    proxy_scheme = environ["wsgi.url_scheme"]
    if environ.get("HTTP_X_FORWARDED_PROTO"):
      proxy_scheme = environ["HTTP_X_FORWARDED_PROTO"]

    proxy_url = proxy_scheme + "://" + environ["HTTP_X_FORWARDED_HOST"]
    proxy_url += urllib.quote(environ.get("REDIRECT_URL", ""))
    proxy_url += urllib.quote(environ.get("PATH_INFO", ""))
  else:
    proxy_url = None

  return server_url, complete_url, proxy_url


def main():
  data = {"service": "WMS", "request": "GetMap"}
  val = GetValue(data, "request")

  print val

if __name__ == "__main__":
  main()
