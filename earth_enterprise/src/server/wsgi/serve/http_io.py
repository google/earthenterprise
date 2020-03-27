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

"""HTTP input/output types.

This module is used to define HTTP request/response objects.
"""
import json
import logging
import re

from common import exceptions
from serve import basic_types
from serve import constants


class Validator(object):
  """Class providing functionality of argument validation."""
  # Note: identifier ::=  (letter|"_") (letter | digit | "_")*
  re_ident = re.compile(r"^[^\d\W]\w*$", re.UNICODE)

  @staticmethod
  def Identifier(val):
    """Validates the value as an identifier.

    Args:
      val: a value to validate.
    Returns:
      the value if valid. Otherwise, returns None.
    """
    match = Validator.re_ident.match(val)
    if match and (len(match.group(0)) == len(val)):
      return match.group(0)
    else:
      logging.warning("Validator - Invalid identifier: '%s'", val)
      return None


class Request(object):
  """Request class."""

  def __init__(self):
    """Inits Request object."""
    self.parameters = {}

  def SetParameter(self, name, value):
    """Sets (parameter:value) pair into request.

    Args:
      name: name of parameter.
      value: value of parameter (integer, string, list).
    """
    if name in self.parameters:
      raise exceptions.StreamPublisherServletException(
          "Internal Error - parameter %s is already set." % name)
    self.parameters[name] = value

  def GetParameter(self, name, default_val=None):
    """Gets parameter value by name.

    Args:
      name: name of parameter.
      default_val: a default value to return if requested parameter is not
                   present.
    Returns:
      parameter value.
    """
    return self.parameters.get(name, default_val)

  def GetBoolParameter(self, name):
    """Gets boolean parameter value by name.

    Args:
      name: name of parameter.
    Returns:
      parameter value.
    """
    val = self.GetParameter(name, default_val=False)
    if val:
      # The case when val is True, non-zero integer, non-empty string.
      return bool(int(val))

    # The case when val is False, zero integer, empty string or None.
    return False

  def GetMultiPartParameter(self, name):
    """Gets multi-part parameter values as sequence.

    Args:
      name: name of parameter.
    Returns:
      sequence of parameter values, empty list if not.
    """
    params = self.GetParameter(name)

    if not params:
      return []

    if isinstance(params, str):
      # Try to parse as multi-part parameter
      # (e.g &FilePath=/tmp/file1,/tmp/file2).
      parameter_list = params.split(constants.MULTI_PART_PARAMETER_DELIMITER)
    else:
      parameter_list = params

    return parameter_list

  def GetIdentifier(self, name, default_val=None):
    """Gets parameter value by name validating it as an identifier.

    Args:
      name: name of parameter.
      default_val: a default value to return if requested parameter is not
                   present.
    Returns:
      parameter value.
    """
    val = self.GetParameter(name, default_val)

    if not val:
      return val

    val = Validator.Identifier(val)

    return val if val else default_val

  def GetClientHostName(self):
    """Returns client host name parameter value.

    Returns:
      Client host name.
    """
    # Get host name parameter that is part of database ID (host, db_name).
    client_host_name = self.GetParameter(constants.HOST_NAME)
    if not client_host_name:
      return client_host_name

    if isinstance(client_host_name, str):
      return client_host_name
    else:
      logging.warning(
          "Internal Error - more then one hostname specified.")
      return client_host_name[0]

  def IsForceCopy(self):
    """Whether 'force copy' is specified in request.

    Returns:
      whether 'force copy' is specified.
    """
    force_copy = False
    force_copy_value = self.GetParameter(constants.FORCE_COPY)
    if force_copy_value and (force_copy_value.capitalize() == "Y"):
      force_copy = True

    return force_copy

  def IsPreferCopy(self):
    """Whether 'prefer copy' is specified in request.

    Returns:
      whether 'prefer copy' is specified.
    """
    prefer_copy = False
    prefer_copy_value = self.GetParameter(constants.PREFER_COPY)
    if prefer_copy_value and (prefer_copy_value.capitalize() == "Y"):
      prefer_copy = True

    return prefer_copy


class Response(object):
  """Response class."""

  def __init__(self):
    """Inits Response object."""
    self.body = []

  def AddBodyElement(self, name, value):
    """Adds body element (name: value) into response.

    Args:
      name: name of body element.
      value: value of body element.
    """
    self.body.append("%s:%s\n\n" % (name, value))

  def AddJsonBody(self, status_code, results):
    """Adds json-formatted body converting results to json-string.

    Converts results to json-formatted string in case it is not a string
    instance, otherwise uses it as is.
    Format: {"status_code": status_code, "results": json_str_of_results_obj)}

    Args:
      status_code: status code of request processing.
      results: object to be added as results into response.
    """
    assert status_code != constants.STATUS_FAILURE
    if isinstance(results, str):
      self.AddJsonStrBody(status_code, results)
    else:
      response = {}
      response[constants.HDR_JSON_STATUS_CODE] = status_code
      response[constants.HDR_JSON_RESULTS] = results
      self.body.append(basic_types.GenericJsonEncoder.DumpJson(response))

  def AddJsonStrBody(self, status_code, results):
    """Adds json-formatted body using results-value as a json-string.

    Builds json-formatted response adding json-formatted string 'results'
    as results.
    Format: {"status_code": status_code, "results": results)}

    Args:
      status_code: status code of request processing.
      results: json-formatted string to be added as results into response.
    """
    assert status_code != constants.STATUS_FAILURE
    assert isinstance(results, str)
    self.body.append("""{"%s": %s, "%s": %s}""" % (
        constants.HDR_JSON_STATUS_CODE, status_code,
        constants.HDR_JSON_RESULTS, results))

  def AddJsonFailureBody(self, status_message):
    """Adds json-formatted failure body into response.

    Args:
      status_message: status message of request processing.
    """
    response = {}
    response[constants.HDR_JSON_STATUS_CODE] = constants.STATUS_FAILURE
    response[constants.HDR_JSON_STATUS_MESSAGE] = status_message
    self.body.append(json.dumps(response))


class ResponseWriter(object):
  """Helper class for writing to a response object."""

  @staticmethod
  def AddBodyElement(response, name, value):
    assert isinstance(response, Response)
    response.AddBodyElement(name, value)

  @staticmethod
  def AddJsonBody(response, status_code, results_obj):
    assert isinstance(response, Response)
    response.AddJsonBody(status_code, results_obj)

  @staticmethod
  def AddJsonStrBody(response, status_code, results):
    assert isinstance(response, Response)
    response.AddJsonStrBody(status_code, results)

  @staticmethod
  def AddJsonFailureBody(response, status_message):
    assert isinstance(response, Response)
    response.AddJsonFailureBody(status_message)
