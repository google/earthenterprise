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

#include "khXML.h"
#include <string>
#include <thread>
#include <mutex>

static khMutexBase xmlLibLock = KH_MUTEX_BASE_INITIALIZER;

bool khXML::doOp(shared_ptr<khXMLOperation> op) throw()
{
  khLockGuard guard(xmlLibLock);
  return op->op();
}


bool khXMLWriteToFile::op(const std::string& filename) throw()
{
}

bool khXMLWriteToFile::op(const std::string& filename, const std::string& unused) throw()
{
	// use of buffer is not allowed here
}

bool khXMLWriteToString::op(const std::string& filename, const std::string& buffer) throw()
{}

bool khXMLWriteToString::op(const std::string& filename) throw()
{
	// buffer is necessary here
}

bool khXMLReadFromFile::op(const std::string& filename) throw()
{}

bool khXMLReadFromFile::op(const std::string& filename, const std::string& buffer) throw()
{
	// use of buffer is not allowed here
}

bool khXMLReadFromString::op(const std::string& filename, const std::string& buffer) throw()
{}

bool khXMLReadFromString::op(const std::string& filename) throw()
{
	// buffer is nessary here
} 
