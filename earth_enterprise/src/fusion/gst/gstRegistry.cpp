// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <ctype.h>
#include <alloca.h>
#include <stdio.h>

#include <qstringlist.h>

#include <notify.h>
#include <gstRegistry.h>

#define MaxBuf 4096

using namespace std;

gstRegistry::gstRegistry(const char* n) {
  _fname = strdupSafe(n);

  _root = new Group(n, NULL);

  _fp = NULL;
  _buff = NULL;
}

gstRegistry::~gstRegistry() {
  delete _root;
}


#if 0
gstStatus gstRegistry::load(const char* buff, int bufflen) {
  if (buff == NULL || bufflen == 0)
    return GST_OPEN_FAIL;

  _buff = buff;
  _bufflen = bufflen;
  _buffpos = 0;
  _fp = NULL;

  return parse();
}
#endif

gstStatus gstRegistry::load() {
  _fp = fopen(name(), "r");

  if (_fp == NULL)
    return GST_OPEN_FAIL;

  _buff = new char[MaxBuf];

  gstStatus retval = parse();

  delete [] _buff;

  fclose(_fp);

  return retval;
}

char* gstRegistry::nextLine() {
  // read from file
  //if (_fp) {
  fgets(_buff, MaxBuf, _fp);
  if (feof(_fp)) {
    return NULL;
  } else {
    while (isspace(_buff[strlen(_buff) - 1]))
      _buff[strlen(_buff) - 1] = '\0';
    return _buff;
  }
  //}

#if 0
  // XXX What!  I don't think this will ever get reached!
  assert(FALSE);

  int currpos = _buffpos;

  while (_buff[_buffpos] != '\n' && _buffpos < _bufflen)
    ++_buffpos;

  if (_buffpos >= _bufflen)
    return NULL;

  _buff[_buffpos] = '\0';
  _buffpos++;

  return &_buff[currpos];
#endif
}

gstStatus gstRegistry::parse() {
  const char OpenSection = '{';
  const char CloseSection = '}';
  const char OpenType = '<';
  const char CloseType = '>';
  const char Assign = ':';
  const char Comment = '#';

  char* tag;
  char* val;
  Group* curGroup = _root;
  int line_num = 0;
  int depth = 0;

  while ((tag = nextLine()) != NULL) {
    ++line_num;

    uint32 varType = gstTagString;

    // skip leading white space
    while (*tag && isspace(*tag))
      ++tag;

    // ignore comments and blank lines
    if (*tag == 0 || *tag == Comment)
      continue;

    if (*tag == CloseSection) {
      depth--;
      if (depth < 0) {
        notify(NFY_WARN, "Syntax error in %s, line %d\n"
               "\t\tTo many end-section markers.", name(), line_num);
        goto error;
      } else {
        curGroup = curGroup->parent();
      }
      // anything on this line after CloseSection is ignored
      continue;
    }

    // Only options at this point are to open a new group,
    // or assign a tag/value pair to the current group

    // Tag/Value pair might be preceeded by a type specifier
    // ie. <int> <float>, etc.

    if (*tag == OpenType) {
      char* type = ++tag;
      while (*tag && *tag != CloseType)
        ++tag;
      if (*tag == CloseType) {
        *tag = 0;
        ++tag;
      } else {
        notify(NFY_WARN, "Syntax error in %s, line %d\n"
               "\t\tType specifier malformed.", name(), line_num);
        goto error;
      }

      varType = gstValue::TypeFromName(type);
      if (varType == gstTagInvalid)
        goto error;

      while (*tag && isspace(*tag))
        ++tag;
    }

    val = tag;
    while (*val && !isspace(*val))
      ++val;
    if (isspace(*val)) {
      *val = 0;
      ++val;
    } else {
      notify(NFY_WARN, "Syntax error in %s, line %d\n"
             "\t\tInvalid group/tag name", name(), line_num);
      goto error;
    }

    while (*val && isspace(*val))
      ++val;

    if (*val == Assign) {
      ++val;

      while (*val && isspace(*val))
        ++val;

      // String values should be enclosed in double quotes
      if (*val != '"') {
        notify(NFY_WARN, "Syntax error in %s, line %d\n"
               "\t\tExpecting opening quote", name(), line_num);
        goto error;
      }

#if 0
      while (*val && val[strlen(val) - 1] != '"')
        val[strlen(val) - 1] = 0;

      if (*val && val[strlen(val) - 1] == '"')
        val[strlen(val) - 1] = 0;
      else {
        notify(NFY_WARN, "Syntax error in %s, line %d\n"
               "\t\tExpecting closing quote", name(), line_num);
        goto error;
      }
#endif

      gstValue* found = curGroup->findTag(tag);

      // create a new one if it doesn't exist (likely case)
      if (!found) {
        found = gstValue::NewValueByType(varType);
        found->SetName(tag);
        curGroup->addTag(found);
      }

      // special-case to allow <CR> in strings
      if (strlenSafe(val) == 1 || val[strlenSafe(val) - 1] != '"') {
        QString bigval(val);
        while ((val = nextLine()) != NULL) {
          bigval += QChar('\n') + QString(val);
          if (strlenSafe(val) && val[strlenSafe(val) - 1] == '"')
            break;
        }
        found->set(bigval.latin1());
      } else {
        found->set(val);
      }
    } else if (*val == OpenSection) {
      Group* found = curGroup->findGroup(tag);
      if (found) {
        curGroup = found;
      } else {
        curGroup = curGroup->addGroup(tag);
      }
      ++depth;
    } else {
      notify(NFY_WARN, "Syntax error in %s, line %d\n"
             "\t\tUnknown state", name(), line_num);
      goto error;
    }
  }

  if (depth != 0) {
    notify(NFY_WARN, "Error in %s, file short.", name());
    goto error;
  }

  _root->resetClear();

  return GST_OKAY;

error:
  return GST_READ_FAIL;
}


#if 0
gstStatus gstRegistry::save()
{
  _fp = fopen(name(), "w");

  if (_fp == NULL)
    return GST_WRITE_FAIL;

  _buff = new char[MaxBuf];

  gstStatus retval = saveGroup(_root, 0);

  fclose(_fp);

  delete [] _buff;

  return retval;
}

gstStatus gstRegistry::save(char **buff, int &bufflen)
{
  _buff = new char[MaxBuf];
  _bufflen = 1024 * MaxBuf;
  _outBuff = (char*)malloc(_bufflen);
  _buffpos = 0;
  _fp = NULL;

  gstStatus retval = saveGroup(_root, 0);

  if (retval != GST_OKAY) {
    free(_outBuff);
    return retval;
  }

  *buff = _outBuff;
  bufflen = _buffpos;

  delete [] _buff;

  return GST_OKAY;
}

int gstRegistry::putLine()
{
  if (_fp) {
    if (fputs(_buff, _fp) == EOF) {
      notify(NFY_WARN, "Unable to save project header!");
      return 0;
    }
    return 1;
  }

  int len = strlen(_buff);

  // make sure buffer is large enough, and grow if needed
  if (_buffpos + len > _bufflen) {
    _bufflen += 1024 * MaxBuf;
    _outBuff = (char*)realloc(_outBuff, _bufflen);
    if (_outBuff == NULL) {
      notify(NFY_WARN, "Unable to write project header, out of memory!");
      return 0;
    }
  }

  memcpy(_outBuff + _buffpos, _buff, len);
  _buffpos += len;
  return 1;
}

gstStatus gstRegistry::saveGroup(Group* group, uint depth) {
  const int tabstop = 2;
  char indent[depth * tabstop + 1];
  indent[0] = 0;

  for (uint ii = 0; ii < depth * tabstop; ++ii)
    strcat(indent, " ");

  for (uint ii = 0; ii < group->tags()->length(); ++ii) {
    gstValue* t = group->tag(ii);
    sprintf(_buff, "%s%s : \"%s\"\n", indent, t->name(), t->ValueAsString());
    if (!putLine())
      return GST_WRITE_FAIL;
  }

  Group** g = group->groups()->array();
  for (uint ii = 0; ii < group->groups()->length(); ++ii, ++g) {
    sprintf(_buff, "%s%s {\n", indent, (*g)->name());
    if (!putLine())
      return GST_WRITE_FAIL;
    if (saveGroup(*g, depth + 1) != GST_OKAY)
      return GST_WRITE_FAIL;
    sprintf(_buff, "%s}\n", indent);
    if (!putLine())
      return GST_WRITE_FAIL;
  }

  return GST_OKAY;
}
#endif


void gstRegistry::setVal(const char* tag, const char *val, uint32 type) {
  gstValue* tagp = locateTag(tag, 1, type);
  assert(tagp != NULL);
  tagp->set(val);
}

#if 0
const char *gstRegistry::getVal(const char *tag, ...) {
  char tagbuf[512];

  va_list args;
  va_start(args, tag);
  vsprintf(tagbuf, tag, args);
  va_end(args);

  gstValue* tagp = locateTag(tagbuf);

  if (tagp)
    return tagp->ValueAsString();

  notify(NFY_DEBUG, "Unknown tag: %s", tagbuf);
  return NULL;
}
#endif

uint gstRegistry::numGroups(const char *tag, ...) {
  char tagbuf[512];

  va_list args;
  va_start(args, tag);
  vsprintf(tagbuf, tag, args);
  va_end(args);

  Group* grp = locateGroup(tagbuf);
  return grp ? grp->numGroups() : -1;
}

uint gstRegistry::numTags(const char *tag, ...) {
  char tagbuf[512];

  va_list args;
  va_start(args, tag);
  vsprintf(tagbuf, tag, args);
  va_end(args);

  Group* grp = locateGroup(tagbuf);
  return grp ? grp->numTags() : -1;
}


// Traverse the registry hierarchy, looking for named tag.
//
// Optionally create any missing groups along the way.
//
// Very little error checking is done.
// Make sure tag points to a valid string.
//
// White space is not skipped, but considered part
//    of the tag path

gstValue *gstRegistry::locateTag(const char *tag, int create, uint32 type) {
  if (tag == NULL || strlen(tag) == 0)
    return NULL;

  // separate tag from group path
  char *itag = (char*)alloca(strlen(tag) + 1);
  strcpy(itag, tag);
  char *sep = strrchr(itag, '/');
  if (sep == NULL)
    return NULL;
  *sep = 0;

  Group* grp = locateGroup(itag, create);
  if (grp == NULL)
    return NULL;

  itag = sep + 1;

  gstValue* found = grp->findTag(itag);

  if (!found && create) {
    found = gstValue::NewValueByType(type);
    found->SetName(itag);
    grp->addTag(found);
  }

  return found;
}

gstRegistry::Group *gstRegistry::locateGroup(const char* tag, int create) {
  Group *tgrp, *grp = _root;

  // make a local copy so we can muck with it
  char* itag = (char*)alloca(strlen(tag) + 1);
  strcpy(itag, tag);

  char* sep;

  while (itag) {
    if ((sep = strchr(itag, '/')) != NULL)
      *sep = 0;

    tgrp = grp->findGroup(itag);
    if (!tgrp) {
      if (create) {
        tgrp = grp->addGroup(itag);
      } else {
        return NULL;
      }
    }
    grp = tgrp;

    if (sep) {
      itag = sep + 1;
    } else {
      itag = NULL;
    }
  }

  return grp;
}

QString gstRegistry::fullPath(Group* grp) {
  QStringList names;
  while (grp) {
    names.push_front(QString::fromUtf8(grp->name()));
    grp = grp->parent();
  }

  QString join = names.join("/");

  return join;
}
