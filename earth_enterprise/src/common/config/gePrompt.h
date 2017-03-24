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

//

#ifndef COMMON_GECONFIGURE_GEPROMPT_H__
#define COMMON_GECONFIGURE_GEPROMPT_H__

#include <string>

class QString;

namespace geprompt {

bool confirm(const char *msg, char dflt = -1);
bool confirm(const std::string &msg, char dflt = -1);
bool confirm(const QString &msg, char dflt = -1);

char choose(const std::string msg, char a, char b, char c = -1,
            char dflt = -1);
int chooseNumber(const std::string msg, int begin, int end);

std::string enterDirname(const std::string msg,
                         const std::string &dflt = std::string(),
                         bool skip_first_prompt = false);
std::string enterString(const std::string &msg,
                        const std::string &dflt = std::string());


} // namespace geprompt

#endif // COMMON_GECONFIGURE_GEPROMPT_H__
