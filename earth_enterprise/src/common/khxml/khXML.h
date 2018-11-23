/*
 * Copyright 2018 the OpenGEE Contributors.
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

#ifndef KHXML_H
#define KHXML_H

#include <memory>

// declare base class for XML operations
class khXMLOperation;

class khXML
{
  khXML() = default;
  khXML(const khXML&) = delete;
  khXML(khXML&&) = delete;
  khXML& operator=(const khXML&) = delete;
  khXML& operator=(khXML&&) = delete;
public:
  khXMLParser() = delete;
  static hXML& instance()
  {
    static khXML _instance;
    return _instance;
  }
	
  static bool doOp(std::unique_ptr<khXMLOperation>);
};

#endif
