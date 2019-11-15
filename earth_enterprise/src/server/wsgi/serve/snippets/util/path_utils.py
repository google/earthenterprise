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


"""Field path (eg 'end_snippet.model.radius') utilities.

* abstract fieldpath: 'a.b.c' - no brackets, certainly no indices.
* empty concrete fieldpath: 'a.b[].c' -- hint that b is repeated, and usually,
  it wants to be assigned an index.
* fully concrete fieldpath: 'a.b[1].c' -- fully instantiated, knows exactly
  where in the proto it lives.
* concrete fieldpath: usually fully concrete, but might be ambiguous;
  either empty_.. or fully_.. .
* semi-concrete fieldpath: 'a.b[2].c.d[].e' - indicates it should be placed or
  should overwrite if need be, b[2], but from d on down will be /appended/ to
  a.b[2].c.d[].

* new-style paths: eg a.b.000003.c.[].d -- prevalent on the client

* old-style paths: eg a.b[3].c[].d -- prevalent on the server.
"""

# TODO: make the paths in one style.
# TODO: clean up, re-factoring.

import re


INDEX_EXPR_RE = re.compile(r"\[\d+\]")
NEWSTYLE_INDEX_RE = re.compile(r"\.\d+\.")
# New and old styles.
OMNI_INDEX_RE = re.compile(r"(\.\d+)|(\.\[\d*\])|(\[\d*\])")


def EnsureFull(path):
  """Prepends 'end_snippet', making it the full field path.

  Args:
      path: the path to ensure is full.
  Returns:
      The path made full.
  """
  if not path.startswith("end_snippet."):
    path = "end_snippet." + path
  return path


def Pretty(path):
  """Removes 'end_snippet.' prefix if present.

  Args:
      path: the path to remove prepended 'end_snippet' from.
  Returns:
      The path de-cluttered with informationless 'end_snippet' prefix.
  """
  if path.startswith("end_snippet."):
    path = path[len("end_snippet."):]
  return path


def SplitNewStyleEmpty(newstyle):
  """Splits according to one of the forms of path elements this code uses.

  See tests/test-path_utils.py for semantics.

  Args:
      newstyle: 'newstyle' path.
  Returns:
      List of tokens from path argument.
  """
  tokens = newstyle.split(".")
  newstyle_tokens = []
  while tokens:
    token = tokens.pop(-1)
    if token == "[]":
      token = tokens.pop(-1) + ".[]"
    newstyle_tokens.insert(0, token)
  return newstyle_tokens


class PathElement(object):
  """Information about a chunk of a path, e.g. 'b' of 'a.b.c'.

  Or, whether 'b' is one of an array, as in 'a.b[2].c'.
  PathElement('a') == a
  PathElement('a', -1) == a[]
  PathElement('a', 3) == a[3]
  """

  def __init__(self, name, index=None):
    self.name = name
    self.index = index

  def IsRepeated(self):
    return self.index is not None

  @staticmethod
  def FromText(chunk_text):
    """Extracts name + index from a single path chunk.

    Handles both old- and new- style paths.

    Args:
        chunk_text: chunk of path to break into name and index.
    Returns:
        PathElement made from path in path.
    """
    if "[" in chunk_text:
      return PathElement.FromOldStyleText(chunk_text)
    else:
      return PathElement.FromNewStyleText(chunk_text)

  @staticmethod
  def FromOldStyleText(chunk_text):
    """Breaks up a single chunk, eg mfe_domain[2] into 'mfe_domain' and 2.

    Args:
        chunk_text: text of path elt.
    Returns:
        PathElement representing chunk_text.
    """
    pos_lbrack = chunk_text.find("[")
    if pos_lbrack < 0:
      return PathElement(chunk_text)
    else:
      pos_rbrack = chunk_text.find("]")
      assert pos_lbrack < pos_rbrack
      index = int(chunk_text[pos_lbrack + 1:pos_rbrack])
      return PathElement(chunk_text[:pos_lbrack], index)

  @staticmethod
  def FromNewStyleText(chunk_text):
    """Breaks up a single chunk, eg mfe_domain.2 into 'mfe_domain' and 2.

    Args:
        chunk_text: the new-style path to extract info from.
    Returns:
        List of PathElements.
    """
    dot = chunk_text.find(".")
    if dot < 0:
      return PathElement(chunk_text)
    else:
      # a.5
      if chunk_text[dot+1].isdigit():
        return PathElement(chunk_text[:dot], int(chunk_text[dot+1:]))
      else:
        assert chunk_text[dot+1] == "["
        pos_lbrack = dot+1
        pos_rbrack = chunk_text.find("]")
        assert pos_lbrack < pos_rbrack
        if pos_lbrack + 1 == pos_rbrack:
          # "[]"
          index = -1
        else:
          index = int(chunk_text[dot+1:pos_rbrack])
        return PathElement(chunk_text[:dot], index)

  @staticmethod
  def IsIndex(tokens):
    return tokens and (tokens[0].isdigit() or tokens[0].startswith("["))

  @staticmethod
  def ElementsFromNewStylePath(path):
    """Breaks a whole path into a list of PathElements.

    Args:
        path: the path to extract info from.
    Returns:
        List of PathElements.
    """
    tokens = path.split(".")
    elt_texts = []
    while tokens:
      elt_text = tokens.pop(0)
      if PathElement.IsIndex(tokens):
        elt_text += "." + tokens.pop(0)
      elt_texts.append(elt_text)
    path_elts = [PathElement.FromNewStyleText(e) for e in elt_texts]
    return path_elts

  @staticmethod
  def ElementAndRestFromOldStylePath(path):
    """Breaks out first chunk of old style path into a PathElement.

    Args:
        path: path to extract from.

    Returns:
        Tuple of PathElement of first chunk, and unprocessed 'rest' of the path.
    """
    tokens = path.split(".", 1)
    element = PathElement.FromText(tokens[0])
    rest = "" if len(tokens) == 1 else tokens[1]
    return element, rest

  @staticmethod
  def ElementAndRestFromNewStylePath(path):
    """Breaks out first chunk of new style path into a PathElement.

    Args:
        path: path to extract from.

    Returns:
        Tuple of PathElement of first chunk, and unprocessed 'rest' of the path.
    """
    element_text = None
    rest = None
    tokens = path.split(".", 2)
    if tokens:
      element_text = tokens.pop(0)
      if PathElement.IsIndex(tokens):
        element_text += "." + tokens.pop(0)

      if tokens:
        rest = ".".join(tokens)

    path_element = PathElement.FromText(element_text)
    return path_element, rest

  def __eq__(self, other):
    return self.name == other.name and self.index == other.index

  def __repr__(self):
    text = self.name
    if self.IsPrepared():
      if self.index < 0:
        text += "[]"
      else:
        text += "[%d]" % self.index
    return text


def AsAbstract(fieldpath):
  """Simplifies a concrete path to an abstract one.

  # catchall; intended to handle any degree or style of concreteness.
  # .d+ blah[d*] .[d*]
  # var paths = ['a.b[2].c',
  #              'a.b[].c',
  #              'a.b.[].c',
  #              'a.b[2].c[]',
  #              'a.b.c[26]',
  #              'a.b.26.c',
  #              'a.b.c.23'];
  # all above produce a.b.c

  'path[0].foo[-1].bar[27].baz' -> path.foo.bar.baz

  Args:
      fieldpath: the path you want the abstract version of.
  Returns:
      Abstract version of the path.
  """
  return re.sub(OMNI_INDEX_RE, "", fieldpath)


def MakeOldStyle(newstyle_fieldpath):
  """New style: a.b.000003.c -> old style: a.b[3].c .

  Args:
      newstyle_fieldpath: new-style path to convert.
  Returns:
      Old-style version of path.
  """
  tokens = newstyle_fieldpath.split(".")
  oldstyle = ""
  is_first = True
  for token in tokens:
    if token.isdigit():
      assert not is_first, "indexes are never first"
      oldstyle += "[%d]" % int(token)
    else:
      oldstyle += token if is_first else "." + token
      is_first = False
  return oldstyle


def IsNewStyle(fieldpath):
  return "[" not in fieldpath


def IsAbstract(fieldpath):
  """a.b[... and a.000003.b both are non-abstract (which would be a.b....).

  Args:
      fieldpath: path of field.
  Returns:
      Whether the field is 'newstyle'.
  """
  abstract = ("[" not in fieldpath) and not NEWSTYLE_INDEX_RE.search(fieldpath)
  return abstract


def SplitAbstract(path):
  """Splits an already 'abstract' path.

  Args:
      path: path to split.
  Returns:
      Text elements of the path.
  """
  return path.split(".")


def FirstRest(tokens):
  """List utility; splits a /list/ robustly into first & rest.

  Args:
      tokens: the list of tokens.
  Returns:
      Tuple of first token, and list of remaining.
  """
  assert isinstance(tokens, list)
  if not tokens:
    first = None
    rest = []
  else:
    first = tokens[0]
    rest = tokens[1:]
  return first, rest


def main():
  #  print PathElement("a", -1)
  #  print PathElement.FromNewStyleText("a.[]")
  print PathElement.ElementsFromNewStylePath("a.b.[]")


if __name__ == "__main__":
  main()
