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


#ifndef FUSION_GECRAWLER_KMLLOG_H__
#define FUSION_GECRAWLER_KMLLOG_H__

#include <string>
#include <sstream>
#include <common/base/macros.h>
#include <khGuard.h>
#include <geFilePool.h>
#include <khTileAddr.h>
#include <khExtents.h>
#include <quadtreepath.h>

namespace gecrawler {

// KmlFolder - hold KML folder data until ready to write to log

class KmlFolder {
 public:
  KmlFolder() {}
  KmlFolder(const QuadtreePath &qt_path,
            const std::string &label);
  ~KmlFolder();
  void Reset(const QuadtreePath &qt_path,
             const std::string &label);
  void AddPoint(const QuadtreePath &qt_path,
                const std::string &style,
                const std::string &description);
  void AddBox(const QuadtreePath &qt_path,
              const std::string &style,
              const std::string &description);
  void Get(std::string *folder_string) const;
  inline bool IsEmpty() { return str_.str().empty(); }
  void Append(const KmlFolder &other);
 private:
  std::ostringstream str_;
  std::string folder_name_;
  khExtents<double> GetDegExtents(const QuadtreePath &qt_path);
  DISALLOW_COPY_AND_ASSIGN(KmlFolder);
};

// KmlLog - write KML formatted log file, viewable in Google Earth.
// This log file shows the differences between the sources.

class KmlLog {
 public:
  static const std::string kSource1Str;
  static const std::string kSource2Str;
  static const std::string kBothStr;

  KmlLog(geFilePool &file_pool, const std::string &log_name);
  ~KmlLog() {}
  void Close();

  enum Source { Source1 = 0, Source2 = 1 };

  void LogQtpDiff(const QuadtreePath &qt_path,
                  const std::string &style,
                  const std::string &label);
  void LogFolder(const KmlFolder &folder);
  void AddDescription(const std::string description);
 private:
  static const std::string kKmlHeader;
  static const std::string kKmlEnd;
  khDeleteGuard<geFilePool::Writer> log_writer_;
  off64_t position_;
  void Append(const std::string &text);
  DISALLOW_COPY_AND_ASSIGN(KmlLog);
};

} // namespace gecrawler

#endif  // FUSION_GECRAWLER_KMLLOG_H__
