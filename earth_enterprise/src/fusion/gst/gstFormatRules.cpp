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


#include <gstFormatRules.h>
#include <notify.h>
#include <vector>
#include <qstringlist.h>
#include <khFileUtils.h>

gstFormatRules *theFormatRules = NULL;


void gstFormatRules::init(void) {
  theFormatRules = new gstFormatRules();
}


gstFormatRules::gstFormatRules(void) {
  gstFormatRule rule;

  rule.name = "Default Roads";

  rule.prefixList.push_back("Mc");

  rule.suppressList.push_back("unnamed street");
  rule.suppressList.push_back("unnamed");
  rule.suppressList.push_back("ramp");
  rule.suppressList.push_back("unknown");
  rule.suppressList.push_back("footbridge");
  rule.suppressList.push_back("connecting");
  rule.suppressList.push_back("unnamed");
  rule.suppressList.push_back("walkway");
  rule.suppressList.push_back("parking");
  rule.suppressList.push_back("driveway");

  rule.allcapsList.push_back("ne");
  rule.allcapsList.push_back("nw");
  rule.allcapsList.push_back("se");
  rule.allcapsList.push_back("sw");
  rule.allcapsList.push_back("nne");
  rule.allcapsList.push_back("ene");
  rule.allcapsList.push_back("ese");
  rule.allcapsList.push_back("sse");
  rule.allcapsList.push_back("ssw");
  rule.allcapsList.push_back("wsw");
  rule.allcapsList.push_back("wnw");
  rule.allcapsList.push_back("nnw");

  ruleSet.rules.push_back(rule);
}

QString gstFormatRules::reformat(const QString &input) const {
  if (input.isEmpty() || ruleSet.rules.size() == 0)
    return input;

  // strip white space at beginning and end
  // replace any sequence of white space with a single space.
  QString outval = input.lower().simplifyWhiteSpace();
  if (outval.isEmpty())
    return outval;

  // someday we will have multiple rules
  // but for now use the first and only rule
  const gstFormatRule &rule = ruleSet.rules[0];

  // throw away any supressed labels
  for (std::vector<QString>::const_iterator it = rule.suppressList.begin();
       it != rule.suppressList.end(); ++it) {
    if (outval == *it)
      return QString();
  }

  QStringList fields = QStringList::split(' ', outval);
  outval.setLength(0);
  for (QStringList::Iterator it = fields.begin(); it != fields.end(); ++it) {
    QString curr_word = *it;
    bool word_done = false;

    // any word that starts with a number should be capitalized
    // QCharRef first = curr_word[ 0 ];
    if (curr_word[ 0 ].isDigit()) {
      curr_word = curr_word.upper();
      word_done = true;
    }

    // capitalize these
    // since this is an exact match, this word is done
    if (!word_done) {
      for (std::vector<QString>::const_iterator cap = rule.allcapsList.begin();
           cap != rule.allcapsList.end(); ++cap) {
        if (curr_word == (*cap)) {
          curr_word = curr_word.upper();
          word_done = true;
          break;
        }
      }
    }

    if (!word_done) {
      QCharRef first = curr_word[0];
      first = first.upper();

      // handle prefixes
      for (std::vector<QString>::const_iterator pfx = rule.prefixList.begin();
           pfx != rule.prefixList.end(); ++pfx) {
        if (curr_word.startsWith(*pfx, false) &&
            curr_word.length() > pfx->length()) {
          QCharRef up = curr_word[pfx->length()];
          up = up.upper();
        }
      }

      // look for hyphens, and capitalize the character immediately following
      int pos = curr_word.find('-');
      while (pos != -1) {
        // point to char immediately after hyphen, testing for end of string
        if (++pos >= curr_word.length())
          break;
        QCharRef up = curr_word[pos];
        up = up.upper();
        // find the next hyphen
        pos = curr_word.find('-', pos);
      }
    }

    if (!outval.isEmpty())
      outval += " ";
    outval += curr_word;
  }

  return outval;
}

