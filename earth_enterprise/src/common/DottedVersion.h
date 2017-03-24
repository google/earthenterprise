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

/* Fusion Dotted Version */

#ifndef COMMON_DOTTEDVERSION_H__
#define COMMON_DOTTEDVERSION_H__

#include <string>
#include <vector>


class DottedVersion {
 public:
  DottedVersion(const std::string &verstr);

  int compare(const DottedVersion &o) const;

  inline bool operator< (const DottedVersion &o) const {
    return compare(o) < 0;
  }
  inline bool operator<= (const DottedVersion &o) const {
    return compare(o) <= 0;
  }
  inline bool operator> (const DottedVersion &o) const {
    return compare(o) > 0;
  }
  inline bool operator>= (const DottedVersion &o) const {
    return compare(o) >= 0;
  }
  inline bool operator== (const DottedVersion &o) const {
    return compare(o) == 0;
  }
  inline bool operator!= (const DottedVersion &o) const {
    return compare(o) != 0;
  }

  std::string ToString(void) const;
 private:
  std::vector<std::string> parts_;
};


inline std::string ToString(const DottedVersion &ver) {
  return ver.ToString();
}




#endif // COMMON_DOTTEDVERSION_H__
