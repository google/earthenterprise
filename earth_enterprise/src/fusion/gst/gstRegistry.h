/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KHSRC_FUSION_GST_GSTREGISTRY_H__
#define KHSRC_FUSION_GST_GSTREGISTRY_H__

#include <gstMisc.h>
#include <khArray.h>
#include <gstValue.h>

#include <qstring.h>

class gstRegistry {
 public:
  class Group {
   public:
    Group(const char* n, Group* g) : _parent(g), _altered(false) {
      _name = strDup(n);
    }

    ~Group() {
      delete [] _name;
      unsigned int ii;
      for (ii = 0; ii < _tags.length(); ++ii) {
        gstValue* t = _tags[ii];
        delete t;
      }
      Group** g = _groups.array();
      for (ii = 0; ii < _groups.length(); ++ii, ++g)
        delete *g;
    }

    const char* name() const { return _name; }
    void name(const char* n) {
      delete [] _name;
      _name = strdupSafe(n);
    }

    Group* parent() const { return _parent; }

    Group* addGroup(const char* n) {
      Group* newgrp = new Group(n, this);
      return addGroup(newgrp);
    }

    Group* addGroup(Group* g) {
      _groups.append(g);
      reset();
      return g;
    }

    khArray<Group*>* groups() { return &_groups; }

    gstValue* addTag(gstValue* t) {
      _tags.append(t);
      reset();
      return t;
    }

    gstValue* removeTag(const char* name) {
      for (unsigned int ii = 0; ii < _tags.length(); ++ii) {
        if (!strcmp(_tags[ii]->Name(), name)) {
          return _tags.removeIndex(ii);
        }
      }
      return nullptr;
    }

    khArray<gstValue*>* tags() { return &_tags; }

    gstValue* findTag(const char* n) const {
      gstValue** t = _tags.array();
      for (unsigned int ii = 0; ii < _tags.length(); ++ii, ++t) {
        if (!strcmpSafe((*t)->Name(), n)) {
          return *t;
        }
      }
      return nullptr;
    }

    Group* findGroup(const char* n) const {
      Group** g = _groups.array();
      for (unsigned int ii = 0; ii < _groups.length(); ++ii, ++g) {
        if (!strcmpSafe((*g)->name(), n)) {
          return *g;
        }
      }
      return nullptr;
    }

    Group* removeGroup(Group* grp) {
      return _groups.remove(grp);
    }

    bool isAltered() { return _altered; }
    void resetClear() { _altered = false; }
    void reset() {
      _altered = true;
      if (_parent)
        _parent->reset();
    }

    int numTags() { return _tags.length(); }
    int numGroups() { return _groups.length(); }
    Group* group(int id) { return _groups[id]; }
    gstValue* tag(int id) { return _tags[id]; }

   private:
    Group* _parent;
    char* _name;

    khArray<gstValue*> _tags;
    khArray<Group*> _groups;

    bool _altered;
  };

  gstRegistry(const char* n = nullptr);
  ~gstRegistry();
  gstRegistry(const gstRegistry&) = delete;
  gstRegistry(gstRegistry&&) = delete;
  gstRegistry& operator=(const gstRegistry&) = delete;
  gstRegistry& operator=(gstRegistry&&) = delete;

  gstStatus load();

  Group* findGroup(const char* g) { return _root->findGroup(g); }

  void setVal(const char*, const char*, std::uint32_t type = gstTagString);

  gstValue* locateTag(const char* tag, int create = 0, std::uint32_t type = gstTagString);
  Group* locateGroup(const char* tag, int create = 0);

  unsigned int numGroups(const char*, ...);

  unsigned int numTags(const char*, ...);

  bool isAltered() { return _root->isAltered(); }

  const char* name() const { return _fname; }
  void setName(const char* n) {
    delete [] _fname;
    _fname = strdupSafe(n);
  }

  Group* root() const { return _root; }
  QString fullPath(Group* g);

 private:

  gstStatus parse();
  char* nextLine();

  Group* _root;
  char* _fname;

  FILE* _fp;
  char* _buff;
  char* _outBuff;
  int _bufflen;
  int _buffpos;
};

#endif  // !KHSRC_FUSION_GST_GSTREGISTRY_H__
