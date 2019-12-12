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


# -*- coding: utf-8 -*-

#
# Generated Wed Jun  8 19:35:12 2011 by generateDS.py version 2.5a.
#

import sys
import getopt
import re as re_

etree_ = None
Verbose_import_ = False
(   XMLParser_import_none, XMLParser_import_lxml,
    XMLParser_import_elementtree
    ) = range(3)
XMLParser_import_library = None
try:
  import defusedxml.ElementTree as etree_
  XMLParser_import_library = XMLParser_elementtree
  if Verbose_import_:
    print("running with defusedxml.ElementTree")
except ImportError:
  try:
    # cElementTree from Python 2.5+
    import defusedxml.cElementTree as etree_
    XMLParser_import_library = XMLParser_import_elementtree
    if Verbose_import_:
      print("running with cElementTree on Python 2.5+")
  except ImportError:
    try:
      # ElementTree from Python 2.5+
      import defusedxml.ElementTree as etree_
      XMLParser_import_library = XMLParser_import_elementtree
      if Verbose_import_:
        print("running with ElementTree on Python 2.5+")
    except ImportError:
      try:
        # normal cElementTree install
        import cElementTree as etree_
        XMLParser_import_library = XMLParser_import_elementtree
        if Verbose_import_:
          print("running with cElementTree")
      except ImportError:
        try:
          # normal ElementTree install
          import elementtree.ElementTree as etree_
          XMLParser_import_library = XMLParser_import_elementtree
          if Verbose_import_:
            print("running with ElementTree")
        except ImportError:
          raise ImportError("Failed to import ElementTree from any known place")

def parsexml_(*args, **kwargs):
  if (XMLParser_import_library == XMLParser_import_lxml and
      'parser' not in kwargs):
    # Use the lxml ElementTree compatible parser so that, e.g.,
    #   we ignore comments.
    kwargs['parser'] = etree_.ETCompatXMLParser()
  doc = etree_.parse(*args, **kwargs)
  return doc

#
# User methods
#
# Calls to the methods in these classes are generated by generateDS.py.
# You can replace these methods by re-implementing the following class
# in a module named generatedssuper.py.

try:
  from generatedssuper import GeneratedsSuper
except ImportError, exp:

  class GeneratedsSuper(object):
    def gds_format_string(self, input_data, input_name=''):
      return input_data
    def gds_validate_string(self, input_data, node, input_name=''):
      return input_data
    def gds_format_integer(self, input_data, input_name=''):
      return '%d' % input_data
    def gds_validate_integer(self, input_data, node, input_name=''):
      return input_data
    def gds_format_integer_list(self, input_data, input_name=''):
      return '%s' % input_data
    def gds_validate_integer_list(self, input_data, node, input_name=''):
      values = input_data.split()
      for value in values:
        try:
          fvalue = float(value)
        except (TypeError, ValueError), exp:
          raise_parse_error(node, 'Requires sequence of integers')
      return input_data
    def gds_format_float(self, input_data, input_name=''):
      return '%f' % input_data
    def gds_validate_float(self, input_data, node, input_name=''):
      return input_data
    def gds_format_float_list(self, input_data, input_name=''):
      return '%s' % input_data
    def gds_validate_float_list(self, input_data, node, input_name=''):
      values = input_data.split()
      for value in values:
        try:
          fvalue = float(value)
        except (TypeError, ValueError), exp:
          raise_parse_error(node, 'Requires sequence of floats')
      return input_data
    def gds_format_double(self, input_data, input_name=''):
      return '%e' % input_data
    def gds_validate_double(self, input_data, node, input_name=''):
      return input_data
    def gds_format_double_list(self, input_data, input_name=''):
      return '%s' % input_data
    def gds_validate_double_list(self, input_data, node, input_name=''):
      values = input_data.split()
      for value in values:
        try:
          fvalue = float(value)
        except (TypeError, ValueError), exp:
          raise_parse_error(node, 'Requires sequence of doubles')
      return input_data
    def gds_format_boolean(self, input_data, input_name=''):
      return '%s' % input_data
    def gds_validate_boolean(self, input_data, node, input_name=''):
      return input_data
    def gds_format_boolean_list(self, input_data, input_name=''):
      return '%s' % input_data
    def gds_validate_boolean_list(self, input_data, node, input_name=''):
      values = input_data.split()
      for value in values:
        if value not in ('true', '1', 'false', '0', ):
          raise_parse_error(node, 'Requires sequence of booleans ("true", "1", "false", "0")')
      return input_data
    def gds_str_lower(self, instring):
      return instring.lower()
    def get_path_(self, node):
      path_list = []
      self.get_path_list_(node, path_list)
      path_list.reverse()
      path = '/'.join(path_list)
      return path
    Tag_strip_pattern_ = re_.compile(r'\{.*\}')
    def get_path_list_(self, node, path_list):
      if node is None:
        return
      tag = GeneratedsSuper.Tag_strip_pattern_.sub('', node.tag)
      if tag:
        path_list.append(tag)
      self.get_path_list_(node.getparent(), path_list)


#
# If you have installed IPython you can uncomment and use the following.
# IPython is available from http://ipython.scipy.org/.
#

## from IPython.Shell import IPShellEmbed
## args = ''
## ipshell = IPShellEmbed(args,
##     banner = 'Dropping into IPython',
##     exit_msg = 'Leaving Interpreter, back to program.')

# Then use the following line where and when you want to drop into the
# IPython shell:
#    ipshell('<some message> -- Entering ipshell.\nHit Ctrl-D to exit')

#
# Globals
#

ExternalEncoding = 'utf8'
Tag_pattern_ = re_.compile(r'({.*})?(.*)')
STRING_CLEANUP_PAT = re_.compile(r"[\n\r\s]+")

#
# Support/utility functions.
#

def showIndent(outfile, level):
  for idx in range(level):
    outfile.write('    ')

def quote_xml(inStr):
  if not inStr:
    return ''
  s1 = (isinstance(inStr, basestring) and inStr or
        '%s' % inStr)
  s1 = s1.replace('&', '&amp;')
  s1 = s1.replace('<', '&lt;')
  s1 = s1.replace('>', '&gt;')
  return s1

def quote_attrib(inStr):
  s1 = (isinstance(inStr, basestring) and inStr or
        '%s' % inStr)
  s1 = s1.replace('&', '&amp;')
  s1 = s1.replace('<', '&lt;')
  s1 = s1.replace('>', '&gt;')
  if '"' in s1:
    if "'" in s1:
      s1 = '"%s"' % s1.replace('"', "&quot;")
    else:
      s1 = "'%s'" % s1
  else:
    s1 = '"%s"' % s1
  return s1

def quote_python(inStr):
  s1 = inStr
  if s1.find("'") == -1:
    if s1.find('\n') == -1:
      return "'%s'" % s1
    else:
      return "'''%s'''" % s1
  else:
    if s1.find('"') != -1:
      s1 = s1.replace('"', '\\"')
    if s1.find('\n') == -1:
      return '"%s"' % s1
    else:
      return '"""%s"""' % s1

def get_all_text_(node):
  if node.text is not None:
    text = node.text
  else:
    text = ''
  for child in node:
    if child.tail is not None:
      text += child.tail
  return text

def find_attr_value_(attr_name, node):
  attrs = node.attrib
  # First try with no namespace.
  value = attrs.get(attr_name)
  if value is None:
    # Now try the other possible namespaces.
    namespaces = node.nsmap.itervalues()
    for namespace in namespaces:
      value = attrs.get('{%s}%s' % (namespace, attr_name, ))
      if value is not None:
        break
  return value


class GDSParseError(Exception):
  pass

def raise_parse_error(node, msg):
  if XMLParser_import_library == XMLParser_import_lxml:
    msg = '%s (element %s/line %d)' % (msg, node.tag, node.sourceline, )
  else:
    msg = '%s (element %s)' % (msg, node.tag, )
  raise GDSParseError(msg)


class MixedContainer:
  # Constants for category:
  CategoryNone = 0
  CategoryText = 1
  CategorySimple = 2
  CategoryComplex = 3
  # Constants for content_type:
  TypeNone = 0
  TypeText = 1
  TypeString = 2
  TypeInteger = 3
  TypeFloat = 4
  TypeDecimal = 5
  TypeDouble = 6
  TypeBoolean = 7
  def __init__(self, category, content_type, name, value):
    self.category = category
    self.content_type = content_type
    self.name = name
    self.value = value
  def getCategory(self):
    return self.category
  def getContenttype(self, content_type):
    return self.content_type
  def getValue(self):
    return self.value
  def getName(self):
    return self.name
  def export(self, outfile, level, name, namespace):
    if self.category == MixedContainer.CategoryText:
      # Prevent exporting empty content as empty lines.
      if self.value.strip():
        outfile.write(self.value)
    elif self.category == MixedContainer.CategorySimple:
      self.exportSimple(outfile, level, name)
    else:    # category == MixedContainer.CategoryComplex
      self.value.export(outfile, level, namespace,name)
  def exportSimple(self, outfile, level, name):
    if self.content_type == MixedContainer.TypeString:
      outfile.write('<%s>%s</%s>' % (self.name, self.value, self.name))
    elif self.content_type == MixedContainer.TypeInteger or \
            self.content_type == MixedContainer.TypeBoolean:
      outfile.write('<%s>%d</%s>' % (self.name, self.value, self.name))
    elif self.content_type == MixedContainer.TypeFloat or \
            self.content_type == MixedContainer.TypeDecimal:
      outfile.write('<%s>%f</%s>' % (self.name, self.value, self.name))
    elif self.content_type == MixedContainer.TypeDouble:
      outfile.write('<%s>%g</%s>' % (self.name, self.value, self.name))
  def exportLiteral(self, outfile, level, name):
    if self.category == MixedContainer.CategoryText:
      showIndent(outfile, level)
      outfile.write('model_.MixedContainer(%d, %d, "%s", "%s"),\n' % \
          (self.category, self.content_type, self.name, self.value))
    elif self.category == MixedContainer.CategorySimple:
      showIndent(outfile, level)
      outfile.write('model_.MixedContainer(%d, %d, "%s", "%s"),\n' % \
          (self.category, self.content_type, self.name, self.value))
    else:    # category == MixedContainer.CategoryComplex
      showIndent(outfile, level)
      outfile.write('model_.MixedContainer(%d, %d, "%s",\n' % \
          (self.category, self.content_type, self.name,))
      self.value.exportLiteral(outfile, level + 1)
      showIndent(outfile, level)
      outfile.write(')\n')


class MemberSpec_(object):
  def __init__(self, name='', data_type='', container=0):
    self.name = name
    self.data_type = data_type
    self.container = container
  def set_name(self, name): self.name = name
  def get_name(self): return self.name
  def set_data_type(self, data_type): self.data_type = data_type
  def get_data_type_chain(self): return self.data_type
  def get_data_type(self):
    if isinstance(self.data_type, list):
      if len(self.data_type) > 0:
        return self.data_type[-1]
      else:
        return 'xs:string'
    else:
      return self.data_type
  def set_container(self, container): self.container = container
  def get_container(self): return self.container

def _cast(typ, value):
  if typ is None or value is None:
    return value
  return typ(value)

#
# Data representation classes.
#

class ServiceExceptionReport(GeneratedsSuper):
  subclass = None
  superclass = None
  def __init__(self, version=None, ServiceException=None):
    self.version = _cast(None, version)
    if ServiceException is None:
      self.ServiceException = []
    else:
      self.ServiceException = ServiceException
  def factory(*args_, **kwargs_):
    if ServiceExceptionReport.subclass:
      return ServiceExceptionReport.subclass(*args_, **kwargs_)
    else:
      return ServiceExceptionReport(*args_, **kwargs_)
  factory = staticmethod(factory)
  def get_ServiceException(self): return self.ServiceException
  def set_ServiceException(self, ServiceException): self.ServiceException = ServiceException
  def add_ServiceException(self, value): self.ServiceException.append(value)
  def insert_ServiceException(self, index, value): self.ServiceException[index] = value
  def get_version(self): return self.version
  def set_version(self, version): self.version = version
  def export(self, outfile, level, namespace_='', name_='ServiceExceptionReport', namespacedef_=''):
    showIndent(outfile, level)
    outfile.write('<%s%s%s' % (namespace_, name_, namespacedef_ and ' ' + namespacedef_ or '', ))
    self.exportAttributes(outfile, level, [], namespace_, name_='ServiceExceptionReport')
    if self.hasContent_():
      outfile.write('>\n')
      self.exportChildren(outfile, level + 1, namespace_, name_)
      showIndent(outfile, level)
      outfile.write('</%s%s>\n' % (namespace_, name_))
    else:
      outfile.write('/>\n')
  def exportAttributes(self, outfile, level, already_processed, namespace_='', name_='ServiceExceptionReport'):
    if self.version is not None and 'version' not in already_processed:
      already_processed.append('version')
      outfile.write(' version=%s' % (self.gds_format_string(quote_attrib(self.version).encode(ExternalEncoding), input_name='version'), ))
  def exportChildren(self, outfile, level, namespace_='', name_='ServiceExceptionReport', fromsubclass_=False):
    for ServiceException_ in self.ServiceException:
      ServiceException_.export(outfile, level, namespace_, name_='ServiceException')
  def hasContent_(self):
    if (
        self.ServiceException
        ):
      return True
    else:
      return False
  def exportLiteral(self, outfile, level, name_='ServiceExceptionReport'):
    level += 1
    self.exportLiteralAttributes(outfile, level, [], name_)
    if self.hasContent_():
      self.exportLiteralChildren(outfile, level, name_)
  def exportLiteralAttributes(self, outfile, level, already_processed, name_):
    if self.version is not None and 'version' not in already_processed:
      already_processed.append('version')
      showIndent(outfile, level)
      outfile.write('version = "%s",\n' % (self.version,))
  def exportLiteralChildren(self, outfile, level, name_):
    showIndent(outfile, level)
    outfile.write('ServiceException=[\n')
    level += 1
    for ServiceException_ in self.ServiceException:
      showIndent(outfile, level)
      outfile.write('model_.ServiceException(\n')
      ServiceException_.exportLiteral(outfile, level)
      showIndent(outfile, level)
      outfile.write('),\n')
    level -= 1
    showIndent(outfile, level)
    outfile.write('],\n')
  def build(self, node):
    self.buildAttributes(node, node.attrib, [])
    for child in node:
      nodeName_ = Tag_pattern_.match(child.tag).groups()[-1]
      self.buildChildren(child, node, nodeName_)
  def buildAttributes(self, node, attrs, already_processed):
    value = find_attr_value_('version', node)
    if value is not None and 'version' not in already_processed:
      already_processed.append('version')
      self.version = value
  def buildChildren(self, child_, node, nodeName_, fromsubclass_=False):
    if nodeName_ == 'ServiceException':
      obj_ = ServiceException.factory()
      obj_.build(child_)
      self.ServiceException.append(obj_)
# end class ServiceExceptionReport


class ServiceException(GeneratedsSuper):
  subclass = None
  superclass = None
  def __init__(self, code=None, valueOf_=None, mixedclass_=None, content_=None):
    self.code = _cast(None, code)
    self.valueOf_ = valueOf_
    if mixedclass_ is None:
      self.mixedclass_ = MixedContainer
    else:
      self.mixedclass_ = mixedclass_
    if content_ is None:
      self.content_ = []
    else:
      self.content_ = content_
    self.valueOf_ = valueOf_
  def factory(*args_, **kwargs_):
    if ServiceException.subclass:
      return ServiceException.subclass(*args_, **kwargs_)
    else:
      return ServiceException(*args_, **kwargs_)
  factory = staticmethod(factory)
  def get_code(self): return self.code
  def set_code(self, code): self.code = code
  def get_valueOf_(self): return self.valueOf_
  def set_valueOf_(self, valueOf_): self.valueOf_ = valueOf_
  def export(self, outfile, level, namespace_='', name_='ServiceException', namespacedef_=''):
    showIndent(outfile, level)
    outfile.write('<%s%s%s' % (namespace_, name_, namespacedef_ and ' ' + namespacedef_ or '', ))
    self.exportAttributes(outfile, level, [], namespace_, name_='ServiceException')
    outfile.write('>')
    self.exportChildren(outfile, level + 1, namespace_, name_)
    outfile.write('</%s%s>\n' % (namespace_, name_))
  def exportAttributes(self, outfile, level, already_processed, namespace_='', name_='ServiceException'):
    if self.code is not None and 'code' not in already_processed:
      already_processed.append('code')
      outfile.write(' code=%s' % (self.gds_format_string(quote_attrib(self.code).encode(ExternalEncoding), input_name='code'), ))
  def exportChildren(self, outfile, level, namespace_='', name_='ServiceException', fromsubclass_=False):
    outfile.write(self.gds_format_string(quote_xml(self.valueOf_).encode(ExternalEncoding)))
  def hasContent_(self):
    if (
        self.valueOf_
        ):
      return True
    else:
      return False
  def exportLiteral(self, outfile, level, name_='ServiceException'):
    level += 1
    self.exportLiteralAttributes(outfile, level, [], name_)
    if self.hasContent_():
      self.exportLiteralChildren(outfile, level, name_)
    showIndent(outfile, level)
    outfile.write('valueOf_ = """%s""",\n' % (self.valueOf_,))
  def exportLiteralAttributes(self, outfile, level, already_processed, name_):
    if self.code is not None and 'code' not in already_processed:
      already_processed.append('code')
      showIndent(outfile, level)
      outfile.write('code = "%s",\n' % (self.code,))
  def exportLiteralChildren(self, outfile, level, name_):
    pass
  def build(self, node):
    self.buildAttributes(node, node.attrib, [])
    self.valueOf_ = get_all_text_(node)
    if node.text is not None:
      obj_ = self.mixedclass_(MixedContainer.CategoryText,
          MixedContainer.TypeNone, '', node.text)
      self.content_.append(obj_)
    for child in node:
      nodeName_ = Tag_pattern_.match(child.tag).groups()[-1]
      self.buildChildren(child, node, nodeName_)
  def buildAttributes(self, node, attrs, already_processed):
    value = find_attr_value_('code', node)
    if value is not None and 'code' not in already_processed:
      already_processed.append('code')
      self.code = value
  def buildChildren(self, child_, node, nodeName_, fromsubclass_=False):
    if not fromsubclass_ and child_.tail is not None:
      obj_ = self.mixedclass_(MixedContainer.CategoryText,
          MixedContainer.TypeNone, '', child_.tail)
      self.content_.append(obj_)
    pass
# end class ServiceException


USAGE_TEXT = """
Usage: python <Parser>.py [ -s ] <in_xml_file>
"""

def usage():
  print USAGE_TEXT
  sys.exit(1)


def get_root_tag(node):
  tag = Tag_pattern_.match(node.tag).groups()[-1]
  rootClass = globals().get(tag)
  return tag, rootClass


def parse(inFileName):
  doc = parsexml_(inFileName)
  rootNode = doc.getroot()
  rootTag, rootClass = get_root_tag(rootNode)
  if rootClass is None:
    rootTag = 'ServiceExceptionReport'
    rootClass = ServiceExceptionReport
  rootObj = rootClass.factory()
  rootObj.build(rootNode)
  # Enable Python to collect the space used by the DOM.
  doc = None
  sys.stdout.write('<?xml version="1.0" ?>\n')
  rootObj.export(sys.stdout, 0, name_=rootTag,
      namespacedef_='')
  return rootObj


def parseString(inString):
  from StringIO import StringIO
  doc = parsexml_(StringIO(inString))
  rootNode = doc.getroot()
  rootTag, rootClass = get_root_tag(rootNode)
  if rootClass is None:
    rootTag = 'ServiceExceptionReport'
    rootClass = ServiceExceptionReport
  rootObj = rootClass.factory()
  rootObj.build(rootNode)
  # Enable Python to collect the space used by the DOM.
  doc = None
  sys.stdout.write('<?xml version="1.0" ?>\n')
  rootObj.export(sys.stdout, 0, name_="ServiceExceptionReport",
      namespacedef_='')
  return rootObj


def parseLiteral(inFileName):
  doc = parsexml_(inFileName)
  rootNode = doc.getroot()
  rootTag, rootClass = get_root_tag(rootNode)
  if rootClass is None:
    rootTag = 'ServiceExceptionReport'
    rootClass = ServiceExceptionReport
  rootObj = rootClass.factory()
  rootObj.build(rootNode)
  # Enable Python to collect the space used by the DOM.
  doc = None
  sys.stdout.write('#from exception import *\n\n')
  sys.stdout.write('import exception as model_\n\n')
  sys.stdout.write('rootObj = model_.rootTag(\n')
  rootObj.exportLiteral(sys.stdout, 0, name_=rootTag)
  sys.stdout.write(')\n')
  return rootObj


def main():
  args = sys.argv[1:]
  if len(args) == 1:
    parse(args[0])
  else:
    usage()


if __name__ == '__main__':
  #import pdb; pdb.set_trace()
  main()


__all__ = [
    "ServiceException",
    "ServiceExceptionReport"
    ]
