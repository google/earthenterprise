#!/usr/bin/env python
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

import os

"""The error handler to generate customized response depending on HTTP status.

It is intended to use with the ErrorDocument directive to supersede a
default error handler. The default error handler returns a query string
in response which may potentially make  the Server vulnerable - an attacker
may be able to cause arbitrary HTML code to be executed in a user's browser
within security context of the affected site.
"""

print "Content-type: text/html"
print
print "<html>"
print "<head>"
print "<title>%s error found</title>" % os.environ.get("REDIRECT_STATUS")
print "</head>"
print "<body>"
print "<h2>Status: %s Condition Intercepted\n</h2>" % os.environ.get("REDIRECT_STATUS")
print "</body>"
print "</html>"
