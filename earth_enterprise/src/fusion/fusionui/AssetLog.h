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



#ifndef _ASSET_LOG_H__
#define _ASSET_LOG_H__

#include "assetlogbase.h"

#include <string>
#include <map>

class AssetLog : public AssetLogBase
{
  AssetLog( const std::string &filename );
 protected:
  ~AssetLog();
 public:

  void timerEvent( QTimerEvent *e );

  static void Open(const std::string &filename);

 private:
  std::string _logfile;
  int _logfd;
  off_t _written;

  typedef std::map<std::string, AssetLog*> LogMap;
  static LogMap openlogs;
};

#endif  // _ASSET_LOG_H__
