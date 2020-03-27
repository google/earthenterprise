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

"""Utility functions for handling snippets masking.

e.g. masking the defaults google.com URLs.
"""

# TODO: clean up, refactoring.

from serve.snippets.data import hard_masked_snippets
from serve.snippets.data import masked_snippets
from serve.snippets.util import dbroot_utils
from serve.snippets.util import path_converters
from serve.snippets.util import path_utils
from serve.snippets.util import proto_reflection
from serve.snippets.util import sparse_tree

# TODO: should fold this google.* into proto_reflection
import google.protobuf.descriptor


def ForcedFieldValues(log):
  """loads + returns fields that are to be forced, with their values.

  Args:
      log: logging obj.
  Returns:
      Set of fields paths user should never see or touch.
  """
  if not ForcedFieldValues.cached:
    ForcedFieldValues.cached = _LoadPathValuesFromDict(
        hard_masked_snippets.hard_masked_snippets, log)
  return ForcedFieldValues.cached
# Simple, full path -> value; no attempt at representing them as a 'tree'.
ForcedFieldValues.cached = {}


def HardSuppressedFields(log):
  """Set, of fields that are hardwired + out of reach of the user.

  Their values are forced into the protobuf; the user never gets to see them.

  Args:
      log: logging obj.
  Returns:
      Set of fields paths user should never see or touch.
  """
  return set(ForcedFieldValues(log).keys())


def NerfedDefaultFieldValues(log):
  """loads + returns the nerfed default field values.

  These are fields which have defaults baked into the protobuf's
  definition which are inconvenient for Google (mostly, urls that
  point at google.com).  This is a list of these, with values to set
  them to, instead. The user can see and change them, and indeed,
  could set them back to 'inconvenient' values, but we prevent that
  from happening, by default.

  Args:
      log: logging obj.
  Returns:
      dict, of nerfed value by fieldpath.
  """
  if not NerfedDefaultFieldValues.cached:
    NerfedDefaultFieldValues.cached = _LoadPathValuesFromDict(
        masked_snippets.masked_snippets, log)
  return NerfedDefaultFieldValues.cached
# Simple, full path -> value; no attempt at representing them as a 'tree'.
NerfedDefaultFieldValues.cached = {}


def _FlexibleBool(bool_text):
  """Tolerant boolean text.

  Args:
      bool_text: the bool text.

  Returns:
      Boolean value of text.

  Raises:
      ValueError: if the text bool isn't recognized.
  """
  as_lower = bool_text.lower()
  if as_lower in _FlexibleBool.TEXT_TRUES:
    return True
  elif as_lower in _FlexibleBool.TEXT_FALSES:
    return False
  else:
    raise ValueError("Can't convert '%s' to bool" % bool_text)
_FlexibleBool.TEXT_TRUES = set(["yes", "y", "true", "t", "on", "1"])
_FlexibleBool.TEXT_FALSES = set(["no", "n", "false", "f", "off", "0"])


def _LoadPathValuesFromDict(snippets, log):
  """Loads {fieldpath: value} from dictionary.

  Verifies field path spelling and demangle it into valid protobuf source
  form.
  a.b.c:value -> a.b.c.value

  Args:
      snippets: snippets path:value map.
      log: logging obj.
  Returns:
      A dict, of value, by fieldpath.
  """
  hardwired_values = {}
  for mangled_path, value in snippets.iteritems():
    # Allow for value being multi-word. <path> of course is 'abstract',
    # ie looks like 'a.b.c', no [] or indices.
    log.debug("field:[%s], value:[%s]", mangled_path, str(value))
    log.debug("maybe mangled: %s", mangled_path)
    path = path_converters.Demangled(mangled_path)
    log.debug("path: %s", path)
    if not proto_reflection.IsLegitFieldPath(path):
      log.warning(
          "The path '%s' is misspelled or otherwise does not exist."
          " Skipping.", path)
      continue

    if not path_utils.IsAbstract(path):
      log.warning(
          "The above path is expected to be abstract (no \'[\', \']\'"
          " or indices).")

    # Assure the end_snippet prefix
    path = path_utils.EnsureFull(path)
    field_type = proto_reflection.TypeAtFieldPath(
        dbroot_utils.MakeEmptyDbroot(), path)

    log.debug("field:[%s], value:[%s]", path, str(value))
    if field_type == google.protobuf.descriptor.FieldDescriptor.TYPE_BOOL:
      if isinstance(value, str):
        value = _FlexibleBool(value)
        log.debug("path getting bool default: %s: %s", path, str(value))

    hardwired_values[path] = value
  return hardwired_values


def RemoveSuppressedFields(store, log):
  """Takes fields out of the sparse tree that is on its way to the client-side.

  Args:
      store: multi-level dict representing the end_snippet.
      log: logging obj.
  """
  # plain {path -> value}, not sparse
  log.debug(">suppress_fields")

  # 'unnerf' - why do we do this...?
  #  _unnerf(store, log)

  for path in HardSuppressedFields(log):
    log.debug("path... " + path)
    log.debug("removing if present... " + path)
    if sparse_tree.HasAbstractFieldPath(path, store):
      # TODO: why do we split finding + removing?
      sparse_tree.RemoveAbstractField(path, store)
  log.debug("<suppress_fields")


def ForceFields(store, log):
  """Writes 'forced' values into the dbroot store.

  The user should not ever have been
  able to see these fields, but we do this just in case and, also, for
  when the dbroot is new.

  Args:
      store: multi-level dict representing the end_snippets protobuf.
      log: logging obj.
  """
  # plain path -> value, not sparse
  path_values = ForcedFieldValues(log)
  # JGD arg; need to gussy up these abstract paths with repeated markers.
  log.debug("have %d fields to force", len(path_values))
  for abstract_path, value in path_values.iteritems():
    log.debug("finding concrete paths for: %s", abstract_path)
    # Just changing the form for the next call.
    empty_concrete_path = proto_reflection.EmptyConcretizeFieldPath(
        abstract_path)
    log.debug("empty_concrete_path: %s", empty_concrete_path)
    if empty_concrete_path:
      sparse_tree.SetAbstractField(empty_concrete_path, value, store, log)


def NerfUnwantedExposedDefaults(store, log):
  """Adds nerfed values to unsafe / inconvenient fields in the 'store'.

  Lit of snippets which default values need to be reseted in proto dbroot.
  Function collects these snippets in store and changes default values to an
  acceptable value.
  The canonical example is urls that point to Google servers, but there are
  others.

  Args:
      store: multi-level dict mimicking the end_snippets protobuf.
      log: logging obj.
  """
  log.debug("Collect and mask unwanted defaults...")
  # plain path -> value, not sparse
  path_values = NerfedDefaultFieldValues(log)
  log.debug("... pathes loaded.")
  for path, value in path_values.iteritems():
    log.debug("..has?" + path)
    if not sparse_tree.HasAbstractFieldPath(path, store):
      log.debug("..set?" + path)
      sparse_tree.SetAbstractField(path, value, store, log)
  log.debug("Masked fields collected.")


def main():
  pass
  #  log = logging_setup.init_logger("snippet-vetting.log", logging.DEBUG)
  #  snippets = _LoadPathValuesFromDict(
  #      hard_masked_snippets.hard_masked_snippets, log)
  #  print snippets
  #
  # just sanity checking. Probably is out-of-date.
  # log = logging_setup.init_logger("snippet-vetting.log", logging.DEBUG)
  # _LoadPathValues(configuration.SNIPPET_NERF_FILEPATH, log)
  # _LoadPathValues(configuration.SNIPPET_HARDMASK_FILEPATH, log)
  # nerf_unwanted_exposed_defaults({}, log)
  # force_fields({}, log)
  # remove_suppressed_fields({}, log)


if __name__ == "__main__":
  main()
