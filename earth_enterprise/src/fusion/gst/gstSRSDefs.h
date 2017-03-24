/*
 * Copyright 2017 Google Inc.
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

#ifndef KHSRC_FUSION_GST_GSTSRSDEFS_H__
#define KHSRC_FUSION_GST_GSTSRSDEFS_H__

#include <vector>
#include <string>

class gstSRSDefs {
 public:
  gstSRSDefs();

  void init();

  class ProjFromEPSG {
   public:
    ProjFromEPSG(const std::string& n, int v)
        : name(n), code(v) {}
    std::string name;
    int code;
  };

  class Category {
   public:
    Category(const std::string& n)
        : name(n) { }
    void addProj(const ProjFromEPSG& p) {
      proj.push_back(p);
    }

    std::string name;
    std::vector<ProjFromEPSG> proj;
  };

  int getCategoryCount() const;
  std::string getCategoryName(int category_id) const;

  std::string getProjectionName(int, int) const;
  int getProjectionCode(int, int) const;
  int getProjectionCount(int category_id) const;

  std::string getWKT(int, int) const;

 private:
  std::vector<Category> srsDefs;
};

extern gstSRSDefs &theSRSDefs;

#endif  // !KHSRC_FUSION_GST_GSTSRSDEFS_H__
