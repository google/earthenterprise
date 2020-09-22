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

#ifndef FUSION_GST_BADFEATUREPOLICY_H__
#define FUSION_GST_BADFEATUREPOLICY_H__

#include <vector>
#include <khException.h>


class SoftErrorPolicy {
 public:
  class TooManyException : public khException {
   public:
    TooManyException(const std::vector<std::string> &errors) :
        khException(kh::tr("Too many soft errors")),
        errors_(errors)
    {
    }
    virtual ~TooManyException(void) throw() { }

    std::vector<std::string> errors_;
  };

  unsigned int NumSoftErrors(void) const { return error_messages_.size(); }
  const std::vector<std::string>& Errors(void) const { return error_messages_; }

  SoftErrorPolicy(unsigned int max_soft_before_fatal);
  void HandleSoftError(const QString &error);

 private:
  const unsigned int max_soft_before_fatal_;
  std::vector<std::string> error_messages_;
};

#endif // FUSION_GST_BADFEATUREPOLICY_H__
