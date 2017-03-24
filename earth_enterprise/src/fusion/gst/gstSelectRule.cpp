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


#include <stdlib.h>
#include <ctype.h>

#include <gstSelectRule.h>

gstSelectRule::gstSelectRule(const SelectRuleConfig& cfg)
    : config(cfg), rvalue_(cfg.rvalue) {

  if ((config.op == SelectRuleConfig::RegExpMatch) ||
      (config.op == SelectRuleConfig::NotRegExpMatch)) {
    regexp = TransferOwnership(new QRegExp(rvalue_.ValueAsUnicode()));
  }
#if 0
  else if ((config.op == SelectRuleConfig::RegExpMatchNoCase) ||
           (config.op == SelectRuleConfig::NotRegExpMatchNoCase)) {
    regexp = TransferOwnership(new QRegExp(rvalue_.ValueAsUnicode(),
                                           false));
  }
#endif
}

int gstSelectRule::eval(gstRecordHandle rec) {
  if (config.fieldNum >= rec->NumFields()) {
    notify(NFY_WARN, "Illegal field number %d, max is %d",
           config.fieldNum, rec->NumFields() - 1);
    return 0;
  }

  gstValue* lval = rec->Field(config.fieldNum);

  switch ( config.op ) {
    case SelectRuleConfig::Equal:
      if (lval->IsString())
        return lval->MatchesWildcard(rvalue_) ? 1 : 0;
      else
        return *lval == rvalue_ ? 1 : 0;
    case SelectRuleConfig::LEqual:   return *lval <= rvalue_ ? 1 : 0;
    case SelectRuleConfig::GEqual:   return *lval >= rvalue_ ? 1 : 0;
    case SelectRuleConfig::Less:     return *lval < rvalue_ ? 1 : 0;
    case SelectRuleConfig::Greater:  return *lval > rvalue_ ? 1 : 0;
    case SelectRuleConfig::NEqual:
      if (lval->IsString())
        return lval->MatchesWildcard(rvalue_) ? 0 : 1;
      else
        return *lval != rvalue_ ? 1 : 0;
    case SelectRuleConfig::RegExpMatch: {
      return (regexp->search(lval->ValueAsUnicode()) >= 0) ? 1 : 0;
    }
    case SelectRuleConfig::NotRegExpMatch: {
      return (regexp->search(lval->ValueAsUnicode()) >= 0) ? 0 : 1;
    }
#if 0
    case SelectRuleConfig::RegExpMatchNoCase: {
      return (regexp->search(lval->ValueAsUnicode()) >= 0) ? 1 : 0;
    }
    case SelectRuleConfig::NotRegExpMatchNoCase: {
      return (regexp->search(lval->ValueAsUnicode()) >= 0) ? 0 : 1;
    }
#endif
  }

  return 0;
}

