// Copyright 2018 The Open GEE Contributors
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


#ifndef __MUTEX_H__
#define __MUTEX_H__


// Required header files
//-----------------------------------------------------------------------------
#include <pthread.h>  // Defines the pthread mutex type
#include <time.h>  // Defines struct timespec


// Namspace used with this class
//-----------------------------------------------------------------------------
namespace thrd {

  // Wrap a 'pthread' timed mutex lock
  //---------------------------------------------------------------------------
  class mutex {

  public:

    mutex();  // ctor
    virtual ~mutex();  // dtor

    static int init();  // called only once to initialize the mutex

    enum { TIME_DELTA_SECONDS = 1 };  // max duration to wait for lock

  protected:

  private:

    static pthread_mutex_t the_mutex;  // the system wide mutex
    static int the_mutex_result;  // hold result of mutex init
    static const timespec time_delta;  // duration to wait before giving up

    int _return_code;

  };  // end class mutex

};  // end NS thrd


#endif  // __MUTEX_H__
