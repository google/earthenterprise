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

//

#include "gePrompt.h"

#include <iostream>
#include <sstream>
#include <khFileUtils.h>
#include <qstring.h>
#include <khException.h>
#include <config/geCapabilities.h>

namespace geprompt {

bool confirm(const char *msg, char dflt) {
  char ch;
  do {
    printf("%s (Y/N)? ", msg);
    if (dflt != -1) {
      printf("[%c] ", dflt);
    }
    char buf[256];
    fgets(buf, sizeof(buf), stdin);
    ch = toupper(buf[0]);
    if (ch == '\n') {
      ch = toupper(dflt);
    }
  } while ((ch != 'Y') && (ch != 'N'));
  return (ch == 'Y');
}

bool confirm(const std::string &msg, char dflt) {
  return confirm(msg.c_str(), dflt);
}

bool confirm(const QString &msg, char dflt) {
  return confirm((const char *)msg.utf8(), dflt);
}

char
choose(const std::string msg, char a, char b, char c, char dflt)
{
  char ch;
  do {
    if (c == -1) {
      printf("%s (%c/%c)? ", msg.c_str(),
             toupper(a), toupper(b));
    } else {
      printf("%s (%c/%c/%c)? ", msg.c_str(),
             toupper(a), toupper(b), toupper(c));
    }
    if (dflt != -1) {
      printf("[%c] ", dflt);
    }
    char buf[256];
    fgets(buf, sizeof(buf), stdin);
    ch = toupper(buf[0]);
    if (ch == '\n') {
      ch = toupper(dflt);
    }
  } while ((ch != toupper(a)) &&
           (ch != toupper(b)) &&
           (ch != toupper(c)));
  return ch;
}

int
chooseNumber(const std::string msg, int begin, int end)
{
  int num;
  do {
    printf("%s (%d - %d)? ", msg.c_str(), begin, end-1);
    char buf[256];
    fgets(buf, sizeof(buf), stdin);
    num = atoi(buf);
  } while ((num < begin) || (num >= end));
  return num;
}

std::string
enterDirname(const std::string msg, const std::string &dflt,
             bool skip_first_prompt)
{
  std::string dir = dflt;
  bool done = false;
  do {
    // prompt the user to enter a directory name
    if (skip_first_prompt) {
      skip_first_prompt = false;
    } else {
      printf("%s: ", msg.c_str());
      if (dir.size()) {
        printf("[%s] ", dir.c_str());
      }
      std::getline(std::cin, dir);
      if (dir.empty())
        dir = dflt;
    }

    // check if it exists and offer to make it for them
    if (!khDirExists(dir)) {
      if (confirm(kh::tr(
"%1 does not exist.\n"
"Should this tool create it now")
                  .arg(dir.c_str()), 'Y')) {
        geCapabilitiesGuard cap_guard(
            CAP_DAC_OVERRIDE,     // let me read all files
            CAP_DAC_READ_SEARCH,  // let me traverse all dirs
            CAP_CHOWN,            // let me chown files
            CAP_FOWNER);          // let me chmod files I dont own
        if (khMakeDir(dir)) {
          done = true;
        }
      }
    } else {
      done = true;
    }
  } while (!done);
  return dir;
}


std::string
enterString(const std::string &msg, const std::string &dflt)
{
  std::string str = dflt;
  printf("%s: ", msg.c_str());
  if (str.size()) {
    printf("[%s] ", str.c_str());
  }
  std::getline(std::cin, str);
  if (str.empty())
    str = dflt;
  return str;
}


} // namespace geprompt
