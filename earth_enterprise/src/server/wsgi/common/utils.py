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

"""utils module.

Module contains common utils.
"""

import logging
import re
import socket
import urllib2
from xml.sax.saxutils import escape
from geAbstractionFetcher import GetHostName

GEHTTPD_CONF_PATH = "/opt/google/gehttpd/conf/gehttpd.conf"
POSTGRES_PROPERTIES_PATH = (
    "/opt/google/gehttpd/wsgi-bin/conf/postgres.properties")


class UrlOpener(object):
  """The urllib2 OpenDirector wrapper.

  Allows to create URL opener that ignores proxy handler that may be specified
  using environment proxy settings (http_proxy variable).
  """

  def __init__(self):
    """Creates URL opener."""
    proxy_handler = urllib2.ProxyHandler({})
    self._opener = urllib2.build_opener(proxy_handler)

  def Open(self, url):
    """Opens the URL url.

    Args:
      url: the URL to open.
    Returns:
      data read from response.
    Raises:
      urllib2.URLError
    """
    fp = self._opener.open(url)
    response = fp.read()
    fp.close()
    return response


def MatchPattern(file_path, pattern):
  """Looks for matching specified pattern in the file line by line.

  Scans through file lines looking for location where specified regular
  expression produces a match.
  Note: checks for a match only at the beginning of the string.
  Args:
    file_path: file path.
    pattern: pattern for matching.
  Returns:
    a tuple containing all the subgroups of the match or None.
  """
  try:
    with open(file_path, "r") as f:
      prog = re.compile(pattern)
      for line in f:
        result = prog.match(line)
        if result:
          return result.groups()
  except IOError:
    pass
  except Exception:
    pass

  return None


def GetSchemeHostPort(environ):
  """Reconstructs scheme, host and port from wsgi environment.

  Args:
    environ: environment variables dictionary.
  Returns:
    reconstructed part of URI: scheme://host:port.
  """
  url = "{0}://".format(environ["wsgi.url_scheme"])

  if environ.get("HTTP_HOST"):
    url += environ["HTTP_HOST"]
  else:
    url += environ["SERVER_NAME"]

    if environ["wsgi.url_scheme"] == "https":
      if environ["SERVER_PORT"] != "443":
        url += ":{0}".format(environ["SERVER_PORT"])
    else:
      if environ["SERVER_PORT"] != "80":
        url += ":{0}".format(environ["SERVER_PORT"])

  return url


def GetServerHost():
  """Gets fully quailified domain name."""
  return GetHostName(True)


def GetApacheServerUrl():
  """Gets server URL (builds self-referential URL) from httpd config.

  Trying to build server URL based on information specified in Apache config
  (Listen, ServerName directives), otherwise build it based on Fully Qualified
  Domain Name and port number specified in Apache config (Listen directive).

  Returns:
    server URL in format scheme://fully-qualified-domain-name[:port]
  """
  scheme_host_port = GetApacheSchemeHostPort()
  if not scheme_host_port:
    return None

  (scheme, host, port) = scheme_host_port
  assert scheme
  assert host
  assert port

  server_url = "{0}://{1}".format(scheme, host)
  if (port and
      ((scheme == "http" and port != "80") or
       (scheme == "https" and port != "443"))):
    server_url += ":{0}".format(port)

  return server_url


def BuildServerUrl(host_):
  """Builds server URL (self-referential URL) for specified host.

  Gets scheme and port number based on information specified in Apache
  config (Listen directive) and builds Server URL based on specified host
  host_ and extracted scheme, port number.
  Args:
    host_: hostname to build URL.
  Returns:
    server URL in format scheme://host_[:port]
  """
  assert host_
  scheme_port = GetApacheSchemePortFromListen()
  if not scheme_port:
    return None

  (scheme, port) = scheme_port[0], scheme_port[1]
  assert scheme
  assert port

  server_url = "{0}://{1}".format(scheme, host_)
  if port and port != "80" and port != "443":
    server_url += ":{0}".format(port)

  return server_url


def GetApacheSchemeHostPort():
  """Gets scheme, host and port number that Apache is running on.

  Gets scheme and port number based on information specified in Apache
  config (Listen, ServerName directives). get host using ServerName or FQDN.

  Returns:
    tuple (scheme, host, port number) that Apache is running on or None.
  """
  # Get (scheme, port) from Listen directive, which is required in httpd
  # config.
  scheme_port = GetApacheSchemePortFromListen()
  if not scheme_port:
    return None

  (scheme, port) = scheme_port
  assert scheme
  assert port

  # Get host from ServerName directive of httpd config, otherwise uses FQDN.
  host = GetApacheServerHost()

  return (scheme, host, port)


def GetApacheSchemePortFromListen():
  """Gets scheme, port number that Apache is running on.

  Gets scheme and port number from Listen directive of httpd config file:
  Format of Listen directive:
  Listen [IP-address:]portnumber [protocol]
  Note: IPv6 addresses must be surrounded in square brackets:
  Listen [2001:db8::a00:20ff:fea7:ccea]:80
  Returns:
    tuple (scheme, port number) that Apache is listening on or None.
  """
  match = MatchPattern(
      GEHTTPD_CONF_PATH,
      r"^Listen\s+(?:\[?([a-fA-F\d\.\:]+)\]?:)?(\d+)(?:\s+(https?))?")
  if match:
    (scheme, port) = (match[2], match[1])
    assert port
    if not scheme:
      scheme = "https" if port == "443" else "http"
    return (scheme, port)

  logging.error("Listen directive is not specified in gehttpd config.")
  return None


def GetApacheServerHost():
  """Gets Apache host from ServerName directive or uses FQDN.

  Trying to get host from ServerName directive of httpd config, otherwise
  uses FQDN.
  Format of ServerName-directive:
  ServerName [scheme://]fully-qualified-domain-name[:port]
  Returns:
    The hostname value.
  """
  host = None
  # Gets (scheme, host, port) from ServerName directive.
  match = MatchPattern(
      GEHTTPD_CONF_PATH,
      r"^ServerName\s+(?:(https?)://)?"
      r"((?:[\da-zA-Z-]+)(?:\.(?:[\da-zA-Z-]+)){1,5})(?::(\d+))?")

  if match and (match[1] not in ["localhost", "127.0.0.1"]):
    host = match[1]

  if not host:
    host = GetServerHost()

  assert host
  return host

# TODO: consider to use a lib like bleach that is specifically
# aimed at foiling XSS attacks.

# Additional characters that need to be escaped for HTML defined in a dictionary
# the character to its escape string.
# xml.sax.saxutils.escape() takes care of &, < and >.
_HTML_ESCAPE_TABLE = {
    '"': "&quot;",
    "'": "&apos;"
    }


def GetPostgresPortNumber():
  """Gets Postgres port number from Port directive of postgres config file.

  Returns:
    Port number to connect to Postgres database server or None.
  """
  pattern = r"^\s*port\s*=\s*(\d{4,})\s*"

  match = MatchPattern(POSTGRES_PROPERTIES_PATH, pattern)
  if match:
    port = match[0]
    return port

  return None


def GetPostgresHost():
  """Get postgres host for remote connections

  Returns:
    Host name of remote database to connect to or None.
  """
  pattern = r"^\s*host\s*=\s*(\d{4,})\s*"

  match = MatchPattern(POSTGRES_PROPERTIES_PATH, pattern)
  if match:
    host = match[0]
    return host

  return None


def GetPostgresPassword():
  """Get geuser role password for remote connections

  Returns:
    Password for remote geuser role or None.
  """
  pattern = r"^\s*pass\s*=\s*(\d{4,})\s*"
  match = MatchPattern(POSTGRES_PROPERTIES_PATH, pattern)
  if match:
    password = match[0]
    return password

  pattern = r"^\s*password\s*=\s*(\d{4,})\s*"
  match = MatchPattern(POSTGRES_PROPERTIES_PATH, pattern)
  if match:
    password = match[0]
    return password

  return None


def HtmlEscape(text):
  """Escapes a string for HTML.

  Args:
    text: source string that needs to be escaped for HTML.
  Returns:
    HTML escaped string.
  """
  return escape(text, _HTML_ESCAPE_TABLE)


def JoinQueryStrings(a, b):
  """Joins query strings.

  Args:
    a : query string.
    b : query string.
  Returns:
    joined query string "{a}&{b}".
  """
  if a and b:
    return "%s&%s" % (a, b)
  elif b:
    return b
  else:
    return a


def main():
  pass


if __name__ == "__main__":
  main()
