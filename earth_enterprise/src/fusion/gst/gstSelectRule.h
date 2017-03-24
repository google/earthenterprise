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

#ifndef KHSRC_FUSION_GST_GSTSELECTRULE_H__
#define KHSRC_FUSION_GST_GSTSELECTRULE_H__

#include <gstValue.h>
#include <gstRegistry.h>
#include <gstMisc.h>
#include <gstRecord.h>
#include <autoingest/.idl/storage/FilterConfig.h>
#include <qregexp.h>

class gstFilter;

class gstSelectRule {
 public:
  gstSelectRule(const SelectRuleConfig& cfg);
  int eval(gstRecordHandle rec);

 private:

  const SelectRuleConfig config;

  // cached outside of config as gstValue since that's what the various
  // comparison routines need
  const gstValue rvalue_;

  // used to bind the regexp only once.
  khDeleteGuard<QRegExp> regexp;
};

#endif  // !KHSRC_FUSION_GST_GSTSELECTRULE_H__
