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

"""Deal with multi-level dict (eg {'a': {'b': 3}}) models of protobufs.

Called /sparse/ because this allows gaps in the repeated parts, which
might be made (or have been deleted) by the user. Because of this
possible 'sparseness', indices of repeated parts are represented as
just numbers, which both Python and JavaScript allow as keys in a
dict.

Look for anything missing from here, in python_model_of_js_paths_store.py.
"""

# TODO: clean up, re-factoring:
# -use integer value as key in dictionary-store for indexing repeated elements.

import traceback

from serve.snippets.util import path_utils


def _AsInt(x):
  """Converts text to int.

  Args:
      x: input value.
  Returns:
      Integer value of x, or None.
  """
  try:
    i = int(x)
    return i
  except (ValueError, TypeError):
    return None


def _CmpNumericIfApplicable(a, b):
  """Compares two strings, as if they were integers.

  Args:
      a: one string.
      b: another.
  Returns:
      0 if a == b, -1 if a < b, 1 if a > b.
  """
  inta = _AsInt(a)
  intb = _AsInt(b)
  if (inta is not None) and (intb is not None):
    return 0 if inta == intb else -1 if inta < intb else 1
  else:
    return cmp(a, b)


def SortedFlattenedSparseTree(path_so_far, tree):
  """Breaks out all paths with values, from the tree / store.

  sparse tree -> [(old-style path, val)] eg:
  {'a': {'b': {'000002': 'xval'}, 'x': 29}}
  ->
  [('a.b[2]', 'xval'), ('a.x', 29)]

  Args:
      path_so_far: current path so far.
      tree: tree store.
  Returns:
      List of tuples of all path, value tuples present in the tree.
  """
  sorted_path_vals = []

  sorted_keys = []
  for key in tree:
    sorted_keys.append(key)
  sorted_keys.sort(cmp=_CmpNumericIfApplicable)

  for key in sorted_keys:
    sub_or_val = tree[key]
    if not path_so_far:
      new_path_so_far = key
    else:
      new_path_so_far = path_so_far + "." + key

    if not isinstance(sub_or_val, dict):
      # We have so much machinery that uses the old style (a.b[x].c)
      # that we convert back.
      oldstyle = path_utils.MakeOldStyle(new_path_so_far)
      sorted_path_vals.append((oldstyle, sub_or_val))
    else:
      sorted_path_vals.extend(SortedFlattenedSparseTree(
          new_path_so_far, sub_or_val))
  return sorted_path_vals


def CompactArrayIndices(tree):
  """Makes a sparse tree dense.

  Returns a tree that may have been sparse, with its indices now compacted.
  Eg 'a.b.0.c', 'a.b.5.c' -> 'a.b.0.c', 'a.b.1.c'.

  Args:
      tree: or store. multi-level dict representing proto dbroot.
  Returns:
      The tree, compacted.
  """
  new_tree = {}
  index = 0
  for key, rest in sorted(tree.iteritems()):
    if key.isdigit():
      key = "%06d" % index
      index += 1
    if isinstance(rest, dict):
      new_tree[key] = CompactArrayIndices(rest)
    else:
      # primitive
      new_tree[key] = rest

  return new_tree


def _RemoveAbstractPathFromRepeatedPartOfSparse(parts, store):
  """Remove path parts from store, when store's top-layer is repeated.

  Args:
      parts: broken-up list of elements of the path.
      store: multi-level dict representing the protobuf dbroot.
  Returns:
      True if the store is non-empty afterward.
  """
  assert isinstance(store, dict)
  indices_to_be_deleted = []
  for index in store:
    assert index.isdigit(), "expected keys to be indices"
    substore = store.get(index, None)
    if isinstance(substore, dict):
      still_lives = _GeneralRemoveAbstractPath(parts, substore)
    else:
      # We've gone down an irrelevant branch; not for us to remove.
      # This is the 'None' case in _omni, where we fail to find
      # the path. We don't try to return whether we found anything
      # to delete or not, because .. that complicates things
      # and we shouldn't really need it.
      still_lives = True

    if not still_lives:
      # Can't delete while traversing.
      indices_to_be_deleted.append(index)

  for index in indices_to_be_deleted:
    del store[index]
  store_lives = bool(store)
  return store_lives


def _GeneralRemoveAbstractPath(parts, store):
  """Remove path parts from store, when store's top-layer is not repeated.

  I believe that removing a whole subtree is just an artifact of
  programmer laziness, not part of its design. Used with
  user-accessible 'template' stuff.  Does no 'required'ness checking;
  use with care.

  Returns true if deletion deleted /something/ (doesn't check the
  whole abstract_path).  It will delete a whole subtree if that
  subtree has no other children (ie is {}), and so on, up. If there is
  only the abstract_path in the tree, will leave an empty tree {}.
  This expects the tree to be 'newstyle', as all sparse trees
  currently are.

  Args:
      parts: the path elements to remove.
      store: store to remove path from.
  Returns:
      Whether we found (and removed) anything.
  """
  # down to parent + child
  assert isinstance(store, dict)
  key, rest = parts[0], parts[1:]
  if not rest:
    # it doesn't matter if what's below is repeated, that just makes it more
    # convenient to destroy them all.
    if key in store:
      del store[key]

    # This deleted 'field' has siblings; the parent should live
    parent_should_live = bool(store)
    return parent_should_live
  else:
    key_should_live = True
    substore = store.get(key, None)
    assert not key.isdigit()
    if not isinstance(substore, dict):
      # Path wasn't in the sparse tree; don't delete anything further up.
      key_should_live = True
    elif substore.keys()[0].isdigit():
      key_should_live = _RemoveAbstractPathFromRepeatedPartOfSparse(
          rest, substore)
    else:
      key_should_live = _GeneralRemoveAbstractPath(rest, substore)

    if not key_should_live:
      del store[key]
      return bool(store)
    else:
      return True


def RemoveAbstractField(abstract_path, store):
  """Remove the /subtree/ of abstract_path.

  See comments in _GeneralRemoveAbstractPath.

  Args:
      abstract_path: the path to remove.
      store: store to remove path from.
  Returns:
      Whether we found (and removed) anything.
  """
  tokens = abstract_path.split(".")
  return _GeneralRemoveAbstractPath(tokens, store)


def HasAbstractFieldPath(abstract_path, store):
  """Whether a store contains abstract_path.

  Makes no provision for repeated fields. I suppose if we did we'd
  have it mean, that /any/ of the repeated subfields had such a
  subpath but, we happen to not need it.

  Args:
      abstract_path: the path to test.
      store: the store.
  Returns:
      Whether the store contains abstract_path.
  """
  tokens = abstract_path.split(".")
  for key in tokens:
    if not isinstance(store, dict) or key not in store:
      return False
    store = store.get(key, {})
  # It's ok (maybe) to even /prematurely/ reach the end of abstract_path.
  return True


def ValueAtFieldPath(fieldpath, store):
  """Gets the value at fieldpath, in store.

  Path can be abstract or 'new-style' indexed (a.b.5.c).

  Args:
      fieldpath: field path; must be new-style.
      store: multi-level dict which the caller thinks contains a fieldpath.
  Returns:
      The value at fieldpath.
  """
  assert path_utils.IsNewStyle(fieldpath), "fieldpath must be newstyle"
  tokens = fieldpath.split(".")
  for key in tokens:
    if not isinstance(store, dict) or key not in store:
      return None
    store = store.get(key, {})

  if isinstance(store, dict):
    return None
  else:
    # Not really a store anymore of course.
    return store


def _SetAbstractThroughRepeated(parts, value, store, log):
  """Set value at parts, when the top of store is 'repeated'.

  Complements _GeneralSetAbstract.
  It is different from normal setting, in that we set corresponding field in
  repeated elements (and their descendants).
  In case we have pre-existing items of repeated element in store,
  we set corresponding field in all the items.
  If there are no items of repeated element in store, we add new item with index
  value of zero and set field in that item. It is the case when corresponding
  repeated snippet is only present in hard-masked snippets list.

  Args:
    parts: list of the elements of the path we're setting.
    value: value we're setting at the path.
    store: the multi-level dict representing our protobuf.
    log: logging obj.
  """
  assert (parts and
          parts[0] == "[]"), "expected element of path to be abstract index."
  assert isinstance(store, dict)

  if store:
    # set field in pre-existing items of repeated element.
    for index in store:
      assert index.isdigit(), "expected store keys to be indexes."
      substore = store.get(index, None)
      if isinstance(substore, dict):
        _GeneralSetAbstract(parts[1:], value, substore, log)
      else:
        # It breaks an invariant of the protobuf => sparse tree, to replace
        # a primitive value with a subtree. Eg, trying to write
        # 'a.b' = 2 to {'a': 1}. 'b' cannot be a sibling of 2. That would be
        # the error we're trying to do here.
        log.error(
            "Programmer or data error; cannot write a subtree ([%s]) over"
            " a primitive value (%s); that\'s an invariant of protobufs =>"
            " sparse trees. Value ignored, but no further damage done. "
            "%s" % (str(index), str(store), repr(traceback.extract_stack())))

  else:
    # Corresponding repeated element is only present in hard-masked snippets
    # list.
    # Note: hard-masked snippets list may only contain one item of repeated
    # element.
    key = "0"  # There are no items so we set index value to zero.
    store[key] = {}
    substore = store[key]
    _GeneralSetAbstract(parts[1:], value, substore, log)


def _GeneralSetAbstract(parts, value, store, log):
  """Set a value in store, where the top level of store is not repeated.

  It complements _SetAbstractThroughRepeated.

  Args:
      parts: already-broken-up path, of where to put the value.
      value: value to write into the store.
      store: the multi-level dict to write into.
      log: logger object
  """
  assert isinstance(store, dict)
  key, rest = path_utils.FirstRest(parts)
  # not concrete
  assert not key.isdigit()
  if not rest:
    store[key] = value
  else:
    substore = store.get(key, {})
    if not substore:
      store[key] = substore

    if isinstance(substore, dict):
      if substore and substore.keys()[0].isdigit():
        # Handle repeated elements.
        _SetAbstractThroughRepeated(rest, value, substore, log)
      else:
        # Get the next element from path to check if it is index of repeated
        # element.
        next_key, unused_rest = path_utils.FirstRest(rest)
        if next_key and next_key == "[]":
          # Handle repeated elements. It is the case when there is no items of
          # repeated element, and first field is coming to set.
          _SetAbstractThroughRepeated(rest, value, substore, log)
        else:
          # Handle generic elements.
          _GeneralSetAbstract(rest, value, substore, log)

    else:
      # It breaks an invariant of the protobuf => sparse tree, to replace
      # a primitive value with a subtree. Eg, trying to write
      # 'a.b' = 2 to {'a': 1}. 'b' cannot be a sibling of 2. That would be
      # the error we're trying to do here.
      log.error("Programmer or data error; cannot write a subtree (%s) over"
                " a primitive value (%s); that\'s an invariant of protobufs =>"
                " sparse trees. Value ignored, but no further damage done. "
                "%s" % (str(key), str(store), repr(traceback.extract_stack())))


def SetAbstractField(path, value, store, log):
  """Sets a value in store, at path.

  Low-level, does no 'required'ness checking.

  Args:
      path: path to set. Is actually theoretically concrete, but in practice
        there are no repeated fields and so concrete == abstract.
      value: value to set.
      store: multi-level dict representing protobuf.
      log: logging obj.
  """
  tokens = path.split(".")
  log.debug("set_abs._field: %s, (%s)" % (path, str(store)))
  # The very top level of end_snippets is not repeated.
  _GeneralSetAbstract(tokens, value, store, log)


def main():
  #  flats = sorted_flattened_sparse_tree("", {
  #      "end_snippet":
  #        {"disable_authentication": True,
  #         "autopia_options":
  #           {"metadata_server_url": "",
  #            "depthmap_server_url": ""},
  #         "elevation_profile_query_delay": 0,
  #         "search_info": {"default_url": ""},
  #         "elevation_service_base_url": "",
  #         "client_options": {"js_bridge_request_whitelist": ""}}})
  pass


if __name__ == "__main__":
  main()
