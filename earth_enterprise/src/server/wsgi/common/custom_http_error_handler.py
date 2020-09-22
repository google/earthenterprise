#!/usr/bin/env python2.7
#
# Copyright 2020 the Open GEE Contributors
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

import io

"""The error handler to generate customized response depending on HTTP status.

It is intended to use with the ErrorDocument directive to supersede a
default error handler. The default error handler returns a query string
in response which may potentially make the Server vulnerable - an attacker
may be able to cause arbitrary HTML code to be executed in a user's browser
within security context of the affected site.
"""


class ErrorHandler(object):
  """Error Handler application class.

  Implements WSGI application interface.
  """

  def __call__(self, environ, start_response):
    """Executes an application.

    Intercepts HTTP responses from Apache ErrorDocument configurations.

    Args:
      environ: WSGI environment.
      start_response: Callable that starts response.
    Returns:
      response body.
    """
    status = environ.get("REDIRECT_STATUS")
    output = io.BytesIO()

    output.write("<html>")
    output.write("<head>")
    output.write("<title>%s error found</title>" % status)
    output.write("</head>")
    output.write("<body>")
    output.write("<h2>Status: %s Condition Intercepted\n</h2>" % status)
    output.write("</body>")
    output.write("</html>")

    response_headers = [('Content-type', 'text/html'),
                        ('Content-Length', str(len(output.getvalue())))]

    start_response(status + ' ', response_headers)
    return [output.getvalue()]


# application instance to use by server.
application = ErrorHandler()

def main():
  pass

if __name__ == "main":
  main()
