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

"""The dbroot managing module.

Functions for writing snippets into proto dbroot.
"""

from serve.snippets.util import configuration
from serve.snippets.util import dbroot_utils
from serve.snippets.util import path_converters
from serve.snippets.util import path_utils
from serve.snippets.util import proto_reflection
from serve.snippets.util import snippet_masker
from serve.snippets.util import sparse_tree
from serve.snippets.util import tree_utils

# TODO: clean up, re-factoring.


class ResultType(object):
  """Result format type for search results."""
  RESULT_TYPE_KML = 0
  RESULT_TYPE_XML = 1


def CreateEndSnippetProto(snippets_json,
                          search_def_list,
                          search_tab_id,
                          supplemental_search_label,
                          supplemental_search_url,
                          log):
  """Creates end snippets proto section from snippets json string.

  Precedence is: filled defaults < user values < forced values.

  Args:
    snippets_json: a set of end snippets in json format.
    search_def_list: a list of search definition objects
        (basic_types.SearchDef) describing search services: label,
        service_url,...
    search_tab_id: a search tab ID to append to a search service label.
        If the search tab id is not None, it is added as a suffix to a search
        service label (format: 'search_service_label [target_path]') to get
        search tab label. Otherwise (None), a search service label is used as
        is.
    supplemental_search_label: a label for the supplemental UI search.
    supplemental_search_url: an URL for the supplemental UI search.
    log: logger object.
  Returns:
    serialized to string proto dbroot content.
  """
  log.debug("CreateEndSnippetProto...")

  dbroot = dbroot_utils.MakeEmptyDbroot()
  log.debug("made empty")

  # TODO a bit inelegant - should write straight to
  # protobuf the 'default-disarming', 'nerfing' values.
  defaults_plugged_tree = {}

  # Overwrite defaults now; the /user's/ values will be written & take
  # precedence, later.
  snippet_masker.NerfUnwantedExposedDefaults(
      defaults_plugged_tree, log)
  log.debug("masked unfriendly...")
  log.debug("defaults plugged:" + str(defaults_plugged_tree))
  _WriteTreeToDbroot(defaults_plugged_tree, dbroot, log)

  # The user's values
  if snippets_json:
    log.debug("writing users snippets")
    users_snippets_tree = tree_utils.CompactTree(snippets_json, log)
    _WriteTreeToDbroot(users_snippets_tree, dbroot, log)

  # Finally, overwrite with any forced values we have.
  forced = {}
  log.debug("writing forced snippets")
  snippet_masker.ForceFields(forced, log)
  log.debug("forced fields")
  _WriteTreeToDbroot(forced, dbroot, log)

  # Add search servers to proto end_snippet.
  _AddSearchServers(dbroot,
                    search_def_list,
                    search_tab_id,
                    supplemental_search_label,
                    supplemental_search_url,
                    log)

  # Note: useful for debugging.
  #  if __debug__:
  #    log.debug("CreateEndSnippetProto - PROTO DBROOT: %s", dbroot)

  content = dbroot.SerializeToString()

  # Note: useful for debugging.
  #  dbroot_restored = dbroot_utils.MakeEmptyDbroot()
  #  dbroot_restored.ParseFromString(content)
  #  dbroot_restored.ParseFromString(content)
  #  log.debug("CreateEndSnippetProto - PROTO DBROOT RESTORED: %s",
  #            dbroot_restored)

  return content


def _WriteTreeToDbroot(tree, proto, log):
  """Writes a tree (store elsewhere) to protobuf.

  Args:
    tree: the tree representing the dbroot
    proto: protobuf to store tree in.
    log: logger obj
  """
  log.debug("writing tree...")
  path_vals = sparse_tree.SortedFlattenedSparseTree("", tree)
  # TODO: it's a shame that we resort to breaking into
  # paths to write to the dbroot - should write tree directly.
  demangled_path_vals = [(path_converters.Demangled(path), val)
                         for path, val in path_vals]

  _SetSnippets(proto, demangled_path_vals, log)
  log.debug("wrote tree")
  if not proto.IsInitialized():
    log.error("bad write, or bad values!")
  else:
    log.debug("wrote path_vals ok")


def _SetSnippets(dbroot_proto, almost_snippet_values, log):
  """Writes the JSON snippet values into the dbroot_proto.

  We write individual paths instead of the whole tree at once,
  only because it's easier.

  Args:
    dbroot_proto: destination dbroot_proto
    almost_snippet_values: list of tuples of dbroot path, value.
      They're 'dense', ie the indexes of all repeated fields all
      start at 0 and are contiguous.
    log: logger object.
  """
  log.debug(">_SetSnippets")
  true_snippet_values = _MassageSpecialCases(almost_snippet_values, log)
  true_snippet_values.sort()

  log.debug("aiming to set in dbroot:")
  for k, v in true_snippet_values:
    assert isinstance(true_snippet_values, list)
    assert isinstance(true_snippet_values[0], tuple)
    log.debug("snippet name:[%s], val:[%s]" % (k, str(v)))
  log.debug("debugged em all")

  for path, value in true_snippet_values:
    log.debug("path: %s value: %s" % (path, value))
  log.debug("setting in binary...")

  proto_reflection.WritePathValsToDbroot(
      dbroot_proto, true_snippet_values, log)
  log.debug("...wrote")
  log.debug("<_SetSnippets")


def _ExtractWidgetEnumTextValues(enums_text, log):
  """Splits the multiple chosen enums from the client.

  We represent dbroot repeated enums as a bunch of checkboxes,
  which is saner for the user.

  Args:
    enums_text: ',' (probably)-separated list of enum
    log: logger obj
  Returns:
  """
  log.debug("converting enums:" + enums_text)
  log.debug("enum separator:>" +
            configuration.WIDGET_ENUM_VALUE_SEPARATOR + "<")
  vals = enums_text.split(configuration.WIDGET_ENUM_VALUE_SEPARATOR)
  log.debug("converting enums ->" + str(vals))
  return vals


def _EnumValFromText(fdesc, enum_text_val, log):
  """Convert text version of enum to integer value.

  Args:
    fdesc: field descriptor containing the text -> int mapping.
    enum_text_val: text to convert.
    log: logger obj
  Returns:
    integer value of enum text.
  """
  log.debug("converting enum val:" + enum_text_val)
  log.debug("possible enum vals:" + str(fdesc.enum_type.values_by_name.keys()))

  enum_val = fdesc.enum_type.values_by_name[enum_text_val.upper()].number
  log.debug("done enum vals")
  return enum_val


def _MassageSpecialCases(almost_snippet_values, log):
  """Catchall for converting anything fiddled-with back to pure dbroot form.

  Right now just translates enum text to numeric values.

  Args:
    almost_snippet_values: snippet values.
    log: logger obj.
  Returns:
    All the snippet values, ready to write to a dbroot.
  """
  log.debug(">massaging...")
  true_snippet_values = []
  assert isinstance(almost_snippet_values, list)
  log.debug("survived assertion")
  dbroot_proto_for_structure = dbroot_utils.MakeEmptyDbroot()
  for concrete_fieldpath, snippet_value in almost_snippet_values:
    log.debug("concrete for no good reason " + concrete_fieldpath)
    abstract_fieldpath = path_utils.AsAbstract(concrete_fieldpath)
    log.debug("abs field:" + abstract_fieldpath)

    fdesc = proto_reflection.FieldDescriptorAtFieldPath(
        dbroot_proto_for_structure, abstract_fieldpath)
    log.debug("got field desc")

    is_enum = fdesc.enum_type is not None
    log.debug("enum? " + str(is_enum))
    # Special case - on the client we represent /repeated enums/ as a choice of
    # fixed checkboxes. Here, we convert that back.
    if not is_enum:
      # plain
      log.debug("massaging, but plain: " + concrete_fieldpath +
                " " + str(snippet_value))
      true_snippet_values.append((concrete_fieldpath, snippet_value))
      log.debug("did not massage non-enum")
    else:
      is_repeated = fdesc.label == fdesc.LABEL_REPEATED
      if not is_repeated:
        log.debug("massaging singular enum:" +
                  concrete_fieldpath + snippet_value)
        enum_val = _EnumValFromText(fdesc, snippet_value, log)
        log.debug("massaged singular; " +
                  concrete_fieldpath + " " + str(enum_val))
        true_snippet_values.append((concrete_fieldpath, enum_val))
      else:
        # repeated enum
        log.debug("supposedly repeated enum...name: %s %s" % (
            concrete_fieldpath, str(snippet_value)))

        enum_text_vals = _ExtractWidgetEnumTextValues(snippet_value, log)

        for i, enum_text in enumerate(enum_text_vals):
          log.debug("enum text: " + enum_text)
          log.debug("all enum vals: " + fdesc.name)
          log.debug("all enum vals: " +
                    str(fdesc.enum_type.values_by_name.keys()))
          enum_val = _EnumValFromText(fdesc, enum_text, log)
          log.debug("whew, found enum val!")
          # need to concretize, now! No way around it.
          log.debug("special enum snippetval: " + snippet_value + "->" +
                    str(enum_val))
          # thank heaven these can only be primitives! (& thus easy to get at)
          added_concrete_fieldpath = concrete_fieldpath + "[%d]" % i
          log.debug("enummed: " + added_concrete_fieldpath)

          true_snippet_values.append((added_concrete_fieldpath, enum_val))
      log.debug("massaged enum")

  log.debug("done massaging")
  return true_snippet_values


def _AddSearchServers(dbroot,
                      search_def_list,
                      search_tab_id,
                      supplemental_search_label,
                      supplemental_search_url,
                      log):
  """Adds search servers into end_snippet proto section.

  Args:
    dbroot: a proto dbroot object to add search servers.
    search_def_list: the list of search definition objects
       (basic_types.SearchDef) describing search services: label,
       service_url,...
    search_tab_id: a search tab ID to append to a search service label.
          If the search tab id is not None, it is added as a suffix to a search
          service label (format: 'search_service_label [target_path]') to get
          search tab label. Otherwise (None), a search service label is used as
          is.
    supplemental_search_label: supplemental_ui search label.
    supplemental_search_url: suplemental_ui search URL.
    log: a logger object.
  """
  if not search_def_list:
    search_server = dbroot.end_snippet.search_config.search_server.add()
    search_server.name.value = ""
    search_server.url.value = "about:blank"
    search_server.html_transform_url.value = "about:blank"
    search_server.kml_transform_url.value = "about:blank"
    search_server.suggest_server.value = "about:blank"
    return

  log.debug("_AddSearchServers()...")
  for search_def in search_def_list:
    log.debug("Configure search server: %s", search_def.label)
    search_server = dbroot.end_snippet.search_config.search_server.add()
    search_server.name.value = (
        "%s [%s]" % (
            search_def.label,
            search_tab_id) if search_tab_id else search_def.label)
    search_server.url.value = search_def.service_url

    # Building query string and appending to search server URL.
    add_query = search_def.additional_query_param
    add_config = search_def.additional_config_param
    query_string = None
    if add_query:
      query_string = ("%s&%s" % (
          query_string, add_query) if query_string else add_query)

    if add_config:
      query_string = ("%s&%s" % (
          query_string, add_config) if query_string else add_config)

    if query_string:
      search_server.url.value = "%s?%s" % (search_server.url.value,
                                           query_string)

    if search_def.fields and search_def.fields[0].suggestion:
      suggestion = search_server.suggestion.add()
      suggestion.value = search_def.fields[0].suggestion

    # Write 'html_transform_url' value to dbroot file.
    search_server.html_transform_url.value = search_def.html_transform_url

    # Write 'kml_transform_url' value to dbroot file.
    search_server.kml_transform_url.value = search_def.kml_transform_url

    # Write 'suggest_server' value to dbroot file.
    search_server.suggest_server.value = search_def.suggest_server

    # Write 'result_type' to dbroot file.
    if search_def.result_type == "XML":
      search_server.type = ResultType.RESULT_TYPE_XML

    # Set supplemental UI properties.
    if supplemental_search_label:
      search_server.supplemental_ui.url.value = supplemental_search_url
      search_server.supplemental_ui.label.value = supplemental_search_label

  log.debug("_AddSearchServers() done.")


def main():
  pass
#  logger = logging.getLogger()
#  snippets_manager = snippets_db_manager.SnippetsDbManager()
#  snippets_set = snippets_manager.GetSnippetSetDetails("a_profile")
#  proto_dbroot = CreateEndSnippetProto(snippets_set, logger)
#  print "PROTO DBROOT bin:"
#  print proto_dbroot


if __name__ == "__main__":
  main()
