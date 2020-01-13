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


"""Utils for converting snippets paths into different internal formats.

Utilities for converting the path a.b.c.value to a.b.c:value and vice versa.
We represent the field's paths that have type StringIDOrValueProto
as 'a.b.c:value', while for other types we have a representation 'a.b.c.value'.
Basically, it is used to differentiate StringIDOrValueProto-type from other
types.

There's a (sub) protobuf, StringIDOrValueProto which is used all over
the place in end_snippet. It supports using either a literal string,
or an index into a table of strings.  It's a flexible design, the
table lookup part is never used in the end_snippet.  So, we hide the
string_id field from the client + thus customer.  When we write those
fields back to the protobuf, though, we need to know that we're doing
that, so we 'mangle' it C++ style with a ':value' (':' normally does
not appear in paths) so that we can treat it specially. Everything
dealing with that is grouped here.
"""

import copy
import re

from serve.snippets.util import configuration

# TODO: clean up, re-factoring.

# If you change the mangling character from ':', remember to
# change the paths in the files with masked snippets set, also.
STRING_PROTO_MANGLED_RE = re.compile(configuration.MANGLING_MARKER + r"value$")
STRING_PROTO_TO_MANGLE_RE = re.compile(r"$")


def Mangled(fieldpath):
  """Marks fieldpath so as to tell us it's a stringidorvalueproto.

  Args:
      fieldpath: field path to mangle.
  Returns:
      The mangled fieldpath.

  Specific to StringIDOrValueProto.
  blah.some_url -> blah.some_url:value
  (because blah.some_url is really: blah.some_url.{ string_id: ?,
  value: ? } and it's the .value part that we use. We don't just hide
  the .value, leaving 'blah.some_url' (we could; we could look at its
  parent's type and know what it was) only because it's more visually
  self-describing this way, and you don't need its type information
  (that it's a StringId...Proto) to know that it needs de-mangling.
  """
  return STRING_PROTO_TO_MANGLE_RE.sub(
      configuration.MANGLING_MARKER + "value",
      fieldpath)


def Demangled(mangled_path):
  """Demangles mangled 'mangled_path' back into valid protobuf fieldpath form.

  Args:
      mangled_path: mangled path.
  Returns:
      valid protobuf fieldpath.
  """
  return STRING_PROTO_MANGLED_RE.sub(".value", mangled_path)


def AsHonoraryPrimitive(path):
  """Human-consumable version of path.

  Ie with neither .value or :value, both just truncated.  So that it
  /looks/ like a primitive.

  Args:
      path: path to clean up for humans.
  Returns:
      path without either .value or :value.
  """
  # Get tough.
  if (path.endswith(".value") or
      path.endswith(configuration.MANGLING_MARKER + "value")):
    # assumes that MANGLING_MARKER is 1 char
    path = path[:-6]
  return path


def IsHonoraryPrimitive(typ):
  return typ in configuration.HONORARY_PRIMITIVES


def ExpandTruncatedFieldPath(fpath):
  return fpath + configuration.MANGLING_MARKER + "value"


def ShortLabelForHonoraryPrimitive(fdesc):
  return fdesc.name


def StringIdOrValueProtoAndRepeatedMarkingMonkeyer(fdesc):
  """Produces data that's friendly to humans from the raw protobuf.

  'Monkeyer' because it is free to do anything (and crosses levels).
  This represents stringidorvalueproto as fieldnames that end in
  ':value', have the default value from ...field.value, and the
  description from ...field (an internal node). Because this is where
  the useful information is in all of our cases.

  Also it looks like it marks up repeated enum fields in a way that's
  distinct from how they'd otherwise be.

  This must operate on a copy of course.

  Really, in the name of transparency and preserving levels of
  abstraction we should produce the tree whole untouched, and only
  then mangle it. The problem is that we'd have to preserve the
  descriptions for the internal nodes, because we use
  StringId....Proto's description, which is internal.

  Args:
    fdesc: snippet field description (row protobuf).
  Returns:
    Human friendly snippet field description.
  """
  copied = copy.deepcopy(fdesc)
  if IsHonoraryPrimitive(copied.typ):
    copied.typ = "string"
    assert not copied.is_proto()
    copied.name = Mangled(fdesc.name)
    value_field = fdesc.proto().field_desc_of("value")
    # pull up .value's default_value; otherwise we're pretty set.
    copied.default_value = value_field.default_value
    copied.short_label = ShortLabelForHonoraryPrimitive(fdesc)

  # Using repeated enums was just a convenience on the part of the
  # proto designers. There can be any number of repeated fields, but
  # only a finite number of enums (duplicates of enums are allowed by
  # the proto).  We hide these non-sensical possibilities from the
  # customer, by treating the field specially. Here, we mark up a
  # repeated enum as though it were a single field (or rather, fail to
  # mark it as repeated).
  if copied.is_repeated() and not copied.is_enum():
    copied.name += ".[]"
  return copied


def main():
  pass


if __name__ == "__main__":
  main()
