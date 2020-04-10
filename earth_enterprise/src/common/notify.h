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


#ifndef _NOTIFY_H
#define _NOTIFY_H

#include <cstdint>
#include <stdarg.h>
#include <stdio.h>
#include <string>

// Notification levels
//
enum khNotifyLevel {
  NFY_FATAL    = 0,  // Fatal error, the dying gasp of a doomed process
  NFY_WARN     = 1,  // Serious warning of bad stuff happening
  NFY_NOTICE   = 2,  // Good things just happened
  NFY_PROGRESS = 3,  // Regular progress messages
  NFY_INFO     = 4,  // Information on progress, goes to notifyFunc()
  NFY_INFO2    = 5,  // Information on progress, goes to notifyFunc()
  NFY_DEBUG    = 6,  // Debug information of significant verbosity
  NFY_VERBOSE  = 7,  // really verbose debug messages
  NFY_DEFAULT_LEVEL = NFY_NOTICE
};

typedef void (*notifyFuncType)(void*, khNotifyLevel, const char*, va_list);
// notify() arguments are checked for consistency with the printf-style format
// string.
extern void notify(khNotifyLevel, const char*, ...)
     __attribute__ ((format (printf, 2, 3)));
extern void setNotifyHandler(notifyFuncType, void*);
extern khNotifyLevel getNotifyLevel();
extern std::string khNotifyLevelToString(khNotifyLevel);
extern khNotifyLevel stringTokhNotifyLevel(const std::string&);
extern void setNotifyLevel(khNotifyLevel);
extern void HexDump(FILE *, const void *, std::uint32_t);
extern std::string khstrerror(int err);
extern std::string GetNotifyPrefixValue(khNotifyLevel severity, const std::string& notify_prefix);


//  ****************************************************************************
//  ***  The default NotifyPrefix is "Fusion"
//  ***  You can change this to whatever you want.
//  ***  If you set it to "[time]" the current timestamp will be used
//  ****************************************************************************
extern std::string NotifyPrefix;
extern const std::string TimePrefix;

#endif  // !_NOTIFY_H
