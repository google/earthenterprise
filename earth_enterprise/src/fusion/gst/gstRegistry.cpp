// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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

  _root = new Group(n, nullptr);

  _fp = nullptr;
  _buff = nullptr;
}

gstRegistry::~gstRegistry() {
  delete _root;
}

gstStatus gstRegistry::load() {
  _fp = fopen(name(), "r");

  if (_fp == nullptr)
    return GST_OPEN_FAIL;

  _buff = new char[MaxBuf];

  gstStatus retval = parse();

  delete [] _buff;

  fclose(_fp);

  return retval;
}

char* gstRegistry::nextLine() {
  // read from file
  fgets(_buff, MaxBuf, _fp);
  if (feof(_fp)) {
    return nullptr;
  } else {
    while (isspace(_buff[strlen(_buff) - 1]))
      _buff[strlen(_buff) - 1] = '\0';
    return _buff;
  }
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

  while ((tag = nextLine()) != nullptr) {
    ++line_num;

    std::uint32_t varType = gstTagString;

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
        while ((val = nextLine()) != nullptr) {
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

void gstRegistry::setVal(const char* tag, const char *val, std::uint32_t type) {
  gstValue* tagp = locateTag(tag, 1, type);
  assert(tagp != nullptr);
  tagp->set(val);
}

unsigned int gstRegistry::numGroups(const char *tag, ...) {
  char tagbuf[512];

  va_list args;
  va_start(args, tag);
  vsprintf(tagbuf, tag, args);
  va_end(args);

  Group* grp = locateGroup(tagbuf);
  return grp ? grp->numGroups() : -1;
}

unsigned int gstRegistry::numTags(const char *tag, ...) {
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

gstValue *gstRegistry::locateTag(const char *tag, int create, std::uint32_t type) {
  if (tag == nullptr || strlen(tag) == 0)
    return nullptr;

  // separate tag from group path
  size_t taglen = strlen(tag) + 1;
  char *itag = (char*)alloca(taglen);
  strncpy(itag, tag, taglen);

  char *sep = strrchr(itag, '/');
  if (sep == nullptr)
    return nullptr;
  *sep = 0;

  Group* grp = locateGroup(itag, create);
  if (grp == nullptr)
    return nullptr;

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
  size_t taglen = strlen(tag) + 1;
  char *itag = (char*)alloca(taglen);
  strncpy(itag, tag, taglen);

  char* sep;

  while (itag) {
    if ((sep = strchr(itag, '/')) != nullptr)
      *sep = 0;

    tgrp = grp->findGroup(itag);
    if (!tgrp) {
      if (create) {
        tgrp = grp->addGroup(itag);
      } else {
        return nullptr;
      }
    }
    grp = tgrp;

    if (sep) {
      itag = sep + 1;
    } else {
      itag = nullptr;
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
