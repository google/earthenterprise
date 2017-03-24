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


/******************************************************************************
File:        ffioPresenceMask.cpp
******************************************************************************/
#include "ffioPresenceMask.h"
#include <autoingest/MiscConfig.h>
#include <khFileUtils.h>


ffio::WaitBase::WaitBase(const std::string &filename)
{
  // since these files can be very small, sometimes kernels and
  // nfs servers in between the writer and the reader can delay the writing.
  // Let's wait until the file is at least a little bit cool. :-)
  WaitIfFileIsTooNew(filename, MiscConfig::Instance().NFSVisibilityDelay);
}


ffio::PresenceMask::PresenceMask(const std::string &filename)
    : WaitBase(filename),
      khPresenceMask(filename)
{ }
