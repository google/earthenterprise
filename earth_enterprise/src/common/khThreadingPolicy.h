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


#ifndef __khThreadingPolicy_h
#define __khThreadingPolicy_h


// ****************************************************************************
// ***  Threading policies are used by various keyhole template classes to
// ***  know if they should perform their work in a thread-safe way or not.
// ***
// ****************************************************************************



// ****************************************************************************
// ***  SingleThreadingPolicy
// ***
// ***  Specifies that the template class need not worry about multiple
// ***  threads accessing the data structure at the same time.
// ***
// ***  The base class MutexHolder consumes zero-space (thanks to C++'s rule
// ***  about empty base classes not adding any more size to a derived
// ***  structure). The LockGuard will hold some space on the stack, but
// ***  since it does absolutely nothing, the optimizer may throw away its
// ***  storage too.
// ****************************************************************************
class SingleThreadingPolicy
{
 public:
  class MutexHolder {
  };
  class LockGuard {
   public:
    LockGuard(const MutexHolder*) { }
  };
};

// ****************************************************************************
// *** MultiThreadingPolicy is defined in khMTTypes.h so files that don't
// *** support threading don't have their headers corrupted by phtreads.h
// ****************************************************************************



#endif /* __khThreadingPolicy_h */
