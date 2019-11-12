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

"""The ModFdbServeHandler module.

Handles Fdb module serving requests by issuing corresponding http request to
mod_fdb service.
"""

import httplib
import logging
import urllib2

from common import exceptions
from common import utils

# Get logger.
logger = logging.getLogger("ge_mod_fdb_serve")


class ModFdbServeHandler(object):
  """Class for handling Fdb module serving requests."""

  def __init__(self):
    # Creates URL opener.
    self._url_opener = utils.UrlOpener()

  def ResetFdbService(self, server_url):
    """Issues reset-request to Fdb module.

    Args:
      server_url: server URL in format scheme://host[:port].
    Raises:
      ModFdbServeException.
    """
    url = "{0}/fdb/reset".format(server_url)
    try:
      response = self._url_opener.Open(url)
    except urllib2.URLError as e:
      if hasattr(e, "code"):  # HTTPError
        raise exceptions.ModFdbServeException(
            "Failed to reset FdbService."
            " Error: %s.\n" % httplib.responses[e.code])
      elif hasattr(e, "reason"):  # URLError
        raise exceptions.ModFdbServeException(
            "Failed to reach a server to reset FdbService."
            " Reason: %s.\n" % e.reason)
    else:
      logger.info(response)

  def RegisterDatabaseForServing(
      self, server_url, target_path, db_type, db_path):
    """Registers FusionDb/Portable for serving in Fdb module.

    Issues request to Fdb module to register FusionDb/Portable for serving.
    Args:
      server_url: server URL in format scheme://host[:port]
      target_path: publish target path.
      db_type: database type.
      db_path: database path.
    Raises:
      ModFdbServeException.
    """
    url = "{0}/fdb/publish?target_path={1}&db_type={2}&db_path={3}".format(
        server_url, target_path, db_type, db_path)

    try:
      response = self._url_opener.Open(url)
    except urllib2.URLError as e:
      if hasattr(e, "code"):  # HTTPError
        raise exceptions.ModFdbServeException(
            "Failed to register for serving target path: %s."
            " Error: %s." % (target_path, httplib.responses[e.code]))
      elif hasattr(e, "reason"):  # URLError
        raise exceptions.ModFdbServeException(
            "Failed to reach a server registering for serving"
            " target path: %s. Reason: %s.\n" % (target_path, e.reason))
    else:
      logger.info(response)

  def UnregisterDatabaseForServing(self, server_url, target_path):
    """Unregisters FusionDb/Portable for serving in Fdb module.

    Issues request to Fdb module to unregister FusionDb/Portable for serving.
    Args:
      server_url: server URL in format scheme://host[:port].
      target_path: publish target path.
    Raises:
      ModFdbServeException.
    """
    url = "{0}/fdb/unpublish?target_path={1}".format(server_url, target_path)
    try:
      response = self._url_opener.Open(url)
    except urllib2.URLError as e:
      if hasattr(e, "code"):  # HTTPError
        raise exceptions.ModFdbServeException(
            "Failed to unregister for serving target path: %s."
            " Error: %s.\n" % (target_path, httplib.responses[e.code]))
      elif hasattr(e, "reason"):  # URLError
        raise exceptions.ModFdbServeException(
            "Failed to reach a server unregistering for serving"
            " target path: %s. Reason: %s.\n" % (target_path, e.reason))
    else:
      logger.info(response)

  def InitCutSpecs(self, server_url, cut_spec_list):
    """Initializes cut specs to allow for dynamic cutting.

    Args:
      server_url: server URL in format scheme://host[:port].
      cut_spec_list: list of cut specifications.
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    logger.info("Initializing cut specs ...")
    for cut_spec in cut_spec_list:
      # If there are exclusion nodes, include them in the request.
      if cut_spec[2]:
        url = (("%s/fdb/add_cut_spec?"
                "name=%s&qtnodes=%s&exclusion_qtnodes=%s&"
                "min_level=%d&default_level=%d&max_level=%d")
               % ([server_url] + cut_spec))
      # Otherwise, skip exclusion nodes.
      else:
        url = (("%s/fdb/add_cut_spec?"
                "name=%s&qtnodes=%s&min_level=%d&default_level=%d&max_level=%d")
               % ([server_url] + cut_spec[0:2] + cut_spec[3:]))
      try:
        response = self._url_opener.Open(url)
      except urllib2.URLError as e:
        if hasattr(e, "code"):  # HTTPError
          logger.warning("Failed to initialize cut spec %s.\nUrl: %s.\n"
                         "Error: %s.",
                         cut_spec[0], url, httplib.responses[e.code])
        elif hasattr(e, "reason"):   # URLError
          logger.warning(
              "Failed to reach a server to initialize cut spec %s.\nUrl: %s\n"
              "Reason: %s.\n", cut_spec[0], url, e.reason)
      else:
        logger.info(response)
        logger.info("Initialized cut spec: %s", cut_spec[0])


def main():
  pass


if __name__ == "__main__":
  main()
