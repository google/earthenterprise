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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <cctype>
#include <sys/types.h>
#include <time.h>

#include <notify.h>

// notify() -- format and report notification message
//
static const char* notifyLevels[] = {
  "Fatal",
  "Warning",
  "Notice",
  "Progress",
  "Info",
  "Info2",
  "Debug",
  "Verbose"
};

// The default level is NFY_NOTICE
// which is set the first time
static notifyFuncType notifyHandler = NULL;
static void* notifyCallData = NULL;

khNotifyLevel getDefaultNotifyLevel() {
  char* variable;
  khNotifyLevel lev;
  if ((variable = getenv("KH_NFY_LEVEL")) != NULL) {
    lev = (khNotifyLevel)atoi(variable);
    if (lev < NFY_WARN) {
      lev = NFY_WARN;
    } else if (lev > NFY_VERBOSE) {
      lev = NFY_VERBOSE;
    }
  } else {
    lev = NFY_DEFAULT_LEVEL;
  }

  return lev;
}

static khNotifyLevel notifyLevel = getDefaultNotifyLevel();

void setNotifyLevel(khNotifyLevel level) {
  notifyLevel = level;
}

khNotifyLevel getNotifyLevel() {
  return notifyLevel;
}

std::string NotifyPrefix("Fusion");
const std::string TimePrefix("[time]");

std::string GetNotifyPrefixValue(khNotifyLevel severity, const std::string& notify_prefix) {
  char prefix_buf[1024];
  if (notify_prefix == TimePrefix) {
    time_t timeval;
    struct tm ts;
    char buf[256];

    time(&timeval);
    if ((timeval != (time_t)-1) &&
        (localtime_r(&timeval, &ts) != 0) &&
        (strftime(buf, sizeof(buf), "%F %T", &ts) > 0)) {
      sprintf(prefix_buf, "[%s] %s: ", buf, notifyLevels[severity]);
    } else {
      sprintf(prefix_buf, "[time] %s: ", notifyLevels[severity]);
    }
  } else {
    sprintf(prefix_buf, "%s %s:\t",
            NotifyPrefix.c_str(), notifyLevels[severity]);
  }
  return std::string(prefix_buf);
}

void notify(khNotifyLevel severity, const char* fmt, ...) {
  // Use the notify handler if any.
  if (notifyHandler != NULL) {
    va_list ap;
    va_start(ap, fmt);
    (*notifyHandler)(notifyCallData, severity, fmt, ap);
    va_end(ap);
  } else if (severity <= notifyLevel) {
    // Process the notify normally.
    va_list ap;
    va_start(ap, fmt);
    std::string prefix = GetNotifyPrefixValue(severity, NotifyPrefix);
    fprintf(stderr, "%s", prefix.c_str());
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    // handle FATAL messages
    if (severity == NFY_FATAL)
      exit(-1);
  }
}

void setNotifyHandler(notifyFuncType handler, void* callData) {
  notifyHandler = handler;
  notifyCallData = callData;
}


void HexDump(FILE* out, const void* data, uint32 size) {
  const char* buf = static_cast<const char*>(data);
  uint32 i = 0;
  while (i < size) {
    static const uint maxrowlen = 20;
    uint rowlen = std::min(maxrowlen, size - i);
    uint j = 0;
    while (j < rowlen) {
      fprintf(out, "%02x ", buf[i+j]);
      ++j;
    }
    while (j < maxrowlen) {
      fputs("   ", out);
      ++j;
    }
    j = 0;
    while (j < rowlen) {
      fprintf(out, "%c", isprint(buf[i+j]) ? buf[i+j] : '.');
      ++j;
    }
    while (j < maxrowlen) {
      fputc(' ', out);
      ++j;
    }
    fputc('\n', out);
    i += rowlen;
  }
}

std::string khstrerror(int err) {
  char buf[256];
  char* msg = strerror_r(err, buf, sizeof(buf));
  if (msg)
    return msg;
  else
    return "Unknown error";
}
