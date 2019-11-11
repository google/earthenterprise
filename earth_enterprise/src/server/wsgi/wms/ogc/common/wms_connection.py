#!/usr/bin/env python2.7
#
# Copyright 2019 Open GEE Contributors
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

"""Handle connections to WMS API"""

import ssl
import urllib2
import urlparse
import logging
import configs

CONFIG_FILE = "/opt/google/gehttpd/wsgi-bin/wms/ogc/wms.cfg"
CONFIGS = configs.Configs(CONFIG_FILE)

logger = logging.getLogger("wms_maps")

def HandleConnection(url):
  logger.debug("Opening url: [%s]", url)

  if CONFIGS.GetStr("DATABASE_HOST") != "":
    url = CONFIGS.GetStr("DATABASE_HOST") + urlparse.urlsplit(url)[2:]

  fp = None
  try:
    # Set the context based on cert requirements
    if CONFIGS.GetBool("VALIDATE_CERTIFICATE"):
      cert_file = CONFIGS.GetStr("CERTIFICATE_CHAIN_PATH")
      key_file = CONFIGS.GetStr("CERTIFICATE_KEY_PATH")
      context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
      context.load_cert_chain(cert_file, keyfile=key_file)
    else:
      context = ssl.create_default_context()
      context.check_hostname = False
      context.verify_mode = ssl.CERT_NONE

    fp = urllib2.urlopen(url, context=context)
    response = fp.read()
  except urllib2.HTTPError, e:
    logger.warning("Server definitions didn't return any results %s.", e)
    return {}
  finally:
    if fp:
      fp.close()

  return response
