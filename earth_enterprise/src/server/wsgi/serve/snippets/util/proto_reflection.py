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


"""Functions to access protobufs via reflection."""

from serve.snippets.util import dbroot_utils
from serve.snippets.util import path_converters
from serve.snippets.util import path_utils

import google.protobuf.descriptor

# TODO: clean up, refactoring: switch to new style path.


def Crash(_):
  assert False, "Trying to convert message or group string, to primitive"


PYTHON_TYPE_BY_PROTOBUF_TYPE = {
    google.protobuf.descriptor.FieldDescriptor.TYPE_DOUBLE:
        float,
    google.protobuf.descriptor.FieldDescriptor.TYPE_FLOAT:
        float,
    google.protobuf.descriptor.FieldDescriptor.TYPE_INT64:
        long,
    google.protobuf.descriptor.FieldDescriptor.TYPE_UINT64:
        long,
    google.protobuf.descriptor.FieldDescriptor.TYPE_INT32:
        int,
    google.protobuf.descriptor.FieldDescriptor.TYPE_FIXED64:
        long,
    google.protobuf.descriptor.FieldDescriptor.TYPE_FIXED32:
        int,
    google.protobuf.descriptor.FieldDescriptor.TYPE_BOOL:
        bool,
    google.protobuf.descriptor.FieldDescriptor.TYPE_STRING:
        str,
    google.protobuf.descriptor.FieldDescriptor.TYPE_GROUP:
        Crash,
    google.protobuf.descriptor.FieldDescriptor.TYPE_MESSAGE:
        Crash,
    google.protobuf.descriptor.FieldDescriptor.TYPE_BYTES:
        str,
    # Technically this should probably be long. should be able to tell.
    google.protobuf.descriptor.FieldDescriptor.TYPE_UINT32:
        int,
    # We do the int conversion earlier EnumTextToValue.
    google.protobuf.descriptor.FieldDescriptor.TYPE_ENUM:
        int,
    google.protobuf.descriptor.FieldDescriptor.TYPE_SFIXED32:
        int,
    google.protobuf.descriptor.FieldDescriptor.TYPE_SFIXED64:
        long,
    google.protobuf.descriptor.FieldDescriptor.TYPE_SINT32:
        int,
    google.protobuf.descriptor.FieldDescriptor.TYPE_SINT64:
        long
}


def StructuralDbroot():
  """Singleton-like empty dbroot as a cache.

  Returns:
    empty dbroot
  """
  if not StructuralDbroot.cached_dbroot:
    StructuralDbroot.cached_dbroot = dbroot_utils.MakeEmptyDbroot()
  return StructuralDbroot.cached_dbroot
StructuralDbroot.cached_dbroot = None


def EnumTextToValue(fdesc, enum_text):
  """Looks up the value of enum_text, given the field descriptor it came from.

  Args:
    fdesc: the descriptor of the field the enum_text belongs to.
    enum_text: the text of an enum.
  Returns:
    integer representing the enum_text
  """
  # descriptor.py
  return fdesc.enum_type.values_by_name[enum_text].number


# fdesc.containing_type -> can get parent DESCRIPTOR
# pbuf: via ListFields() can get fdesc + value -- if not repeated
# pbuf.DESCRIPTOR.fields_by_name.iteritems() -> name, value) -- name, value
# pdesc.fields -> fdesc
# https://developers.google.com/protocol-buffers/docs/reference/python/
#    google.protobuf.descriptor.Descriptor-class
# https://developers.google.com/protocol-buffers/docs/reference/python/
#    google.protobuf.descriptor.FieldDescriptor-class
def _NavigateToField(protobuf, field_path, log=None):
  """Navigates via structure (ie protobuf can be empty).

  Args:
    protobuf: message from with the path starts.
    field_path: path of field to navigate to.
    log: Python logging object
  Returns:
    content of the field.
  """
  if log:
    log.debug("_NavigateToField: . . .")
  field_elt, rest_of_field_path = (
      path_utils.PathElement.ElementAndRestFromOldStylePath(field_path))
  if not rest_of_field_path:
    if log:
      log.debug("_navigate - reached primitive")
    # caller has to get / set the value at the field path_toks[0]
    return protobuf
  else:
    fielddesc = protobuf.DESCRIPTOR.fields_by_name[field_elt.name]
    assert fielddesc
    assert fielddesc.type == fielddesc.TYPE_MESSAGE, (
        "an internal node must be a proto")
    is_repeated = fielddesc.label == fielddesc.LABEL_REPEATED
    if log:
      log.debug("_navigate: is repeated? %s %s" % (
          str(is_repeated), fielddesc.name))
    if is_repeated:
      if log:
        log.debug("_navigate: repeated; index: %d" % field_elt.index)
        log.debug("_navigate: repeated; of len: %d" %
                  len(getattr(protobuf, field_elt.name)))
      repeated = getattr(protobuf, field_elt.name)
      # TODO: This is a bit rough and will get us
      # into trouble someday.
      if field_elt.index < 0:
        field_elt.index = 0
      # no more than 1 above the len
      if field_elt.index == len(getattr(protobuf, field_elt.name)):
        # abstract (ie no index given)? Make it 0; we /need/ an index when
        # it's for something repeated, even if the proto is actually empty.
        # Being purely 'structural' fails (is there no other way?)
        repeated.add()
      assert fielddesc.type == fielddesc.TYPE_MESSAGE
      assert getattr(protobuf, field_elt.name)

    subbuf = None
    if is_repeated:
      if log:
        log.debug("_navigate: getting %dth field" % field_elt.index)
      subbuf = getattr(protobuf, field_elt.name)[field_elt.index]
    else:
      if log:
        log.debug("_navigate: getting plain field")
      subbuf = getattr(protobuf, field_elt.name)
    return _NavigateToField(subbuf, rest_of_field_path, log)


def SetValueAtFieldPath(protobuf, field_path, maybe_text_value, log):
  """Set value at fieldpath, in protobuf.

  Field_path here is old-style, a.b[x].c.

  Args:
    protobuf: container in which to set value
    field_path: path at which to set value
    maybe_text_value: value. If it's text, it's converted to its true type
      (we know from fieldpath).
    log: Python logging object
  """
  value = maybe_text_value
  if isinstance(value, basestring):
    typ = TypeAtFieldPath(protobuf, field_path, log)
    if (not value and
        (typ != google.protobuf.descriptor.FieldDescriptor.TYPE_STRING and
         typ != google.protobuf.descriptor.FieldDescriptor.TYPE_BYTES)):
      field_desc = FieldDescriptorAtFieldPath(protobuf, field_path)
      value = field_desc.default_value
    else:
      # Use the type as a conversion function.
      value = PYTHON_TYPE_BY_PROTOBUF_TYPE[typ](maybe_text_value)

  owner = _NavigateToField(protobuf, field_path, log)
  fieldname = field_path.rsplit(".", 1)[-1]
  pelt = path_utils.PathElement.FromText(fieldname)

  # Singular
  if not pelt.IsRepeated():
    try:
      setattr(owner, pelt.name, value)
    except BaseException as e:
      log.error(str(e))
  # Repeated
  else:
    # No 'fill-in' loop on primitives; we'd have to use defaults,
    # and we don't really need it anyway. The internal filling-in
    # of '...navigate...' is just gravy.
    if len(getattr(owner, pelt.name)) < pelt.index:
      if log:
        log.error("about to fail because:%d < idx %d" % (
            len(getattr(owner, pelt.name)), pelt.index))
      assert False
    # if it's the last one ... ?
    elif len(getattr(owner, pelt.name)) - 1 == pelt.index:
      getattr(owner, pelt.name)[pelt.index] = value
    else:
      getattr(owner, pelt.name).append(value)


def GetValueAtFieldPath(protobuf, fieldpath, log):
  """Gets value of field at fieldpath in protobuf.

  Is unused, but it's so central it has to come in handy someday.

  Args:
    protobuf: container of the protobuf
    fieldpath: path of the field within the protobuf
    log: Python logging object.
  Returns:
    The value at fieldpath
  """
  fieldname = fieldpath.rsplit(".", 1)[-1]
  pelt = path_utils.PathElement.FromText(fieldname)

  field_owner = _NavigateToField(protobuf, fieldpath, log)
  if pelt.is_repeated():
    return getattr(field_owner, pelt.name)[pelt.index]
  else:
    return getattr(field_owner, pelt.name)


def FieldDescriptorAtFieldPath(protobuf, fieldpath, log=None):
  """Gets field descriptor of field at fieldpath.

  Args:
    protobuf: protobuf in which to find field.
    fieldpath: path to field.
    log: Python logging object.
  Returns:
    field descriptor of field at fieldpath.
  """
  fieldname = fieldpath.rsplit(".", 1)[-1]
  owner = _NavigateToField(protobuf, fieldpath, log)
  fdesc = owner.DESCRIPTOR.fields_by_name.get(fieldname, None)
  if log:
    log.debug(fieldname + ": got fdesc")
  return fdesc


def TypeAtFieldPath(protobuf, fieldpath, logf=None):
  """The protobuf enummed type.

  See PYTHON_TYPE_BY_PROTOBUF_TYPE at the top.

  Args:
    protobuf: protobuf in which to find field.
    fieldpath: path to field.
    logf: log file object.
  Returns:
    Field type at fieldpath.
  """
  fdesc = FieldDescriptorAtFieldPath(protobuf, fieldpath, logf)
  return fdesc.type


def EmptyConcretizeFieldPath(fieldpath, protobuf=None):
  """Adds empty concreteness markings to an abstract fieldpath.

  Takes an abstract path (no indices or '[]') and puts '[]' markers at
  repeated parts, newstyle (ie a.b.[]). We do this for reading from
  the template, masking file.

  Args:
    fieldpath: path of the field within the protobuf
    protobuf: container of the protobuf
  Returns:
    fieldpath, with empty concrete markings.
  """
  assert path_utils.IsAbstract(fieldpath)
  if protobuf is None:
    protobuf = StructuralDbroot()
  toks = path_utils.SplitAbstract(fieldpath)
  empty_concrete_toks = []

  def AddElement(fname, fd):
    empty_concrete_toks.append(fname)
    if fd.label == fd.LABEL_REPEATED:
      empty_concrete_toks.append("[]")

  def GetNext(toks, fd):
    fname, rest = path_utils.FirstRest(toks)
    fd = fd.message_type.fields_by_name.get(fname, None)
    if fd is None:
      assert False, "bad path: %s, at: %s" % (fieldpath, fname)
    return fname, rest, fd

  fname, rest = path_utils.FirstRest(toks)
  fd = protobuf.DESCRIPTOR.fields_by_name.get(fname, None)
  if fd is None:
    assert False, "bad path: %s, at: %s" % (fieldpath, fname)

  AddElement(fname, fd)
  while bool(rest) and fd.type == fd.TYPE_MESSAGE:
    fname, rest, fd = GetNext(rest, fd)
    AddElement(fname, fd)

  assert (not rest) and (fd.type != fd.TYPE_MESSAGE), (
      "path %s incomplete or something" % fieldpath)
  return ".".join(empty_concrete_toks)


def _IsFieldPathPrimitive(fieldpath, protobuf):
  """Whether the fieldpath is a (true) leaf in protobuf.

  whether protobuf structurally contains the path and, that it ends
  at a primitive (ie rejects subtree paths).

  Args:
    fieldpath: path of the field within the protobuf
    protobuf: container of the protobuf
  Returns:
    whether the fieldpath ends at a true leaf ie, value field.
  """
  toks = path_utils.SplitAbstract(path_utils.AsAbstract(fieldpath))
  fname, rest = path_utils.FirstRest(toks)
  fd = protobuf.DESCRIPTOR.fields_by_name.get(fname, None)
  if fd is None:
    return False
  while bool(rest) and fd.type == fd.TYPE_MESSAGE:
    fname, rest = path_utils.FirstRest(rest)
    fd = fd.message_type.fields_by_name.get(fname, None)
    if fd is None:
      # path contains something not in the protobuf
      return False
  # Both ended on a primitive.
  return (not rest) and (fd.type != fd.TYPE_MESSAGE)


def FieldpathNature(fieldpath, protobuf=None):
  """Returns information about the nature of a given field.

  Args:
    fieldpath: path of the field of interest
    protobuf: container of the field.
  Returns:
    tuple of: (whether <fieldpath> is contained in the protobuf,
    is true-or-honorary primitive, is strictly honorary primitive)
  """
  if protobuf is None:
    protobuf = StructuralDbroot()
  toks = path_utils.SplitAbstract(path_utils.AsAbstract(fieldpath))
  fname, rest = path_utils.FirstRest(toks)
  fd = protobuf.DESCRIPTOR.fields_by_name.get(fname, None)
  if fd is None:
    return (False, None, None)
  while bool(rest) and fd.type == fd.TYPE_MESSAGE:
    fname, rest = path_utils.FirstRest(rest)
    fd = fd.message_type.fields_by_name.get(fname, None)
    if fd is None:
      # path contains something not in the protobuf
      return (False, None, None)
  # 'poked through the bottom' of the protobuf
  if rest:
    return (False, None, None)
  is_true_primitive = fd.type != fd.TYPE_MESSAGE
  is_primitive_like = is_true_primitive or (
      path_converters.IsHonoraryPrimitive(fd.message_type.name))
  return (True, is_primitive_like, not is_true_primitive)


def _IsFieldPathPrimitiveLike(fieldpath, protobuf):
  """Whether fieldpath is an actual primitive, or honorary primitive.

  We allow this latter, for our doc writers, so they don't have to
  fiddle + maybe get wrong, adding :value to the end of so much.

  Args:
    fieldpath: path to field.
    protobuf: protobuf in which to find field.
  Returns:
    whether the field at fieldpath is a primitive (true leaf) or, an
    honorary primitive.
  """
  toks = path_utils.SplitAbstract(path_utils.AsAbstract(fieldpath))
  fname, rest = path_utils.FirstRest(toks)
  fd = protobuf.DESCRIPTOR.fields_by_name.get(fname, None)
  if fd is None:
    return False
  while bool(rest) and fd.type == fd.TYPE_MESSAGE:
    fname, rest = path_utils.FirstRest(rest)
    fd = fd.message_type.fields_by_name.get(fname, None)
    if fd is None:
      # path contains something not in the protobuf
      return False
  # Ended of path
  return (not rest) and (
      # .. is primitive
      fd.type != fd.TYPE_MESSAGE or
      # .. or honorary primitive
      path_converters.IsHonoraryPrimitive(fd.message_type.name))


def IsLegitFieldPath(abstract_path, protobuf=None):
  """Whether the abstract_path is in protobuf.

  Honoraries are considered 'legit' here.

  Args:
    abstract_path: path of the field within the protobuf.
    protobuf: container of the protobuf.
  Returns:
    Whether the path is either a true, or honorary primitive in protobuf.
  """
  if protobuf is None:
    protobuf = StructuralDbroot()
  (is_contained, is_primitive_like, unused_strictly_honorary) = (
      FieldpathNature(abstract_path))
  return is_contained and is_primitive_like


def WritePathValsToDbroot(dbroot, fieldpath_vals, log=None):
  """Writes an iterable of path, value pairs to dbroot.

  Args:
    dbroot: in which to write the values.
    fieldpath_vals: tuples of path, value to write.
    log: Python logging object.
  """
  if fieldpath_vals:
    for concrete_fieldpath, val in fieldpath_vals:
      if log:
        log.debug("writing path:>%s<, >%s<" % (concrete_fieldpath, str(val)))
      SetValueAtFieldPath(dbroot, concrete_fieldpath, val, log)
      if log:
        log.debug("done writing path")

  if log:
    log.debug("done writing path vals")


def main():
  pass


if __name__ == "__main__":
  main()
