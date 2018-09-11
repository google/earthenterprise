// Copyright 2017 Google Inc. Copyright 2018 the Open GEE Contributors.
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


#include "fusion/autoingest/JsUtils.h"
#include <sstream>
#include <array>

std::string JsUtils::JsToJson(const std::string& in)
{
    std::ostringstream oss;

    const std::string alphaNumSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789-abcdefghijklmnopqrstuvwxyz.";
    const std::string valueSet = "0123456789.eE";  // .eE should only appear once but let something else enforce that

    for (size_t i = 0; i < in.length(); ++i) {
        if (isalpha(in[i])) {
            static const std::array<std::string, 3> jsonLiterals = {"true", "false", "null"};
            bool isLiteral = false;
            for (const std::string& jlit : jsonLiterals) {
                if (!jlit.compare(in.substr(i, jlit.size()))) {
                    oss << jlit;
                    i += jlit.size()-1;
                    isLiteral = true;
                }
            }

            if (!isLiteral) {
                size_t endEscape = in.find_first_not_of(alphaNumSet, i);
                oss << "\"" << in.substr(i, endEscape-i) << "\"";
                i = endEscape - 1;
            }
        }
        else if (isdigit(in[i])) {
            size_t endValue = in.find_first_not_of(valueSet, i);
            oss << in.substr(i, endValue-i);
            i = endValue - 1;
        }
        else if (in[i] == '\n') {
            // these may not hurt but removing them for now to declutter
            continue;
        }
        else if (in[i] == '\"') {
            size_t escapedStrEnd = in.find("\"", i+1);
            std::string temp = in.substr(i, escapedStrEnd-i+1);
            oss << in.substr(i, escapedStrEnd-i+1);
            i = escapedStrEnd;
        }
        else {
            oss << in[i];
        }
    }

    return oss.str();
}
