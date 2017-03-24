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


"""Attempt at PyWms for Portable. Service for serving requests to /ext/... path.

As of 2013-01-18 does not work, though its structure should be mostly
correct.

The service defines three methods: SetWorkingDirectory,
ExtGetHandler, and ExtPostHandler.

SetWorkingDirectory sets the directory where this file is located and allows
the service to easily find external files that it may want to use to respond
to requests.

ExtGetHandler handles a GET request to the /ext/... pathway. Arguments for
the request are passed in via the handler object. The relative path
after '/ext/' is also passed in as a parameter.

ExtGetHandler handles a POST request to the /ext/... pathway. Arguments for
the request are passed in via the handler object. The relative path
after '/ext/' is also passed in as a parameter.
"""

import datetime
import os
import sys

# TODO: - get the directories + paths straightened out.
sys.path.append('ogc')

import gee
import utils
import wms


class ExtService(object):
  """Class for serving requests to /ext/... path."""

  def __init__(self):
    print 'wms ExtService'

  def SetWorkingDirectory(self, working_directory):
    """Use the given base directory to find any associated files."""
    # Set the location of our geplaces data.
    self.working_directory_ = working_directory
    print 'Ext working directory is %s' % self.working_directory_

  def ExtGetHandler(self, handler):
    """Service a GET request to the /ext/... pathway."""

    parameters = dict([(k, v[0])
                       for k, v in handler.request.arguments.iteritems()])
    print 'wms params:', parameters
    request = handler.request
    print 'TIDY REQUEST:', (request.protocol, request.host,
                            request.path, request.uri)
    # endpoint is:
    #   http://localhost/cgi-bin/ogc/service.py
    # ie everything before the '?'
    print 'joined:', os.path.join(request.host, request.path)
    parameters['this-endpoint'] = (request.protocol + '://' +
                                   os.path.join(
                                       request.host, 'maps'
                                       ))
    print 'get url?'
    url = utils.GetValue(parameters, 'url') or parameters['this-endpoint']
    print 'url:[%s]' % url
    parameters['server-obj'] = gee.PortableServer(url)
    print 'made server - gen response?'
    status, header_pairs, output = wms.GenerateResponse(parameters)
    print 'supposedly generated the response'
    handler.set_status(status)
    for header_pair in header_pairs:
      handler.add_header(header_pair[0], header_pair[1])
    handler.write(output)
    # not sure whether needed.
    handler.flush()

  def ExtPostHandler(self, handler, path):
    """Service a POST request to the /ext/... pathway."""
    handler.set_header('Content-Type', 'text/html')
    handler.write('<p>Process POST request...')
    try:
      handler.write('<p>Hello %s!' % handler.request.arguments['name'][0])
    except KeyError:
      handler.write('<p>Hello unknown!')
    handler.write('<p>Path: %s' % path)

    handler.write('<p>Time: %s' % datetime.datetime.now().ctime())
