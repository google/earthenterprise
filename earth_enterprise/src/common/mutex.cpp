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


// Required header files
//-----------------------------------------------------------------------------
#include <stdexcept>  // std::runtime_error
#include "common/mutex.h"  // defines class thrd::mutex


// Namespace short hand
//-----------------------------------------------------------------------------
using namespace std;  // standard library NS
using namespace thrd;  // Local NS for this class


// Static member function
// Called only once to initialize the system wide mutex variable
//-----------------------------------------------------------------------------
int mutex::init() { return pthread_mutex_init( &mutex::the_mutex, NULL ); };


// Static class variables
//-----------------------------------------------------------------------------
pthread_mutex_t mutex::the_mutex;  // system wide mutex
int mutex::the_mutex_result = mutex::init();  // init the mutex
const timespec mutex::time_delta = { mutex::TIME_DELTA_SECONDS, 0 };  // delay for 1 second


// Class constructor
// Attempt to lock the mutex.  Wait mutex::time_delta before giving up.
//-----------------------------------------------------------------------------
mutex::mutex() : _return_code( 0x0 ) {

  static const string message( "Mutex lock failure" );  // simple error message

  // system function returns 0 if mutex was locked, error code otherwise
  _return_code = pthread_mutex_timedlock( &mutex::the_mutex, &time_delta );

  // If failed to lock mutex throw an exception
  if ( _return_code != 0x0 ) throw runtime_error( message );

};  // end ctor



// Class destructor
// Check the valued returned when the system wide mutex was locked.
// If the returned value is 0 then unlock the mutex.
//-----------------------------------------------------------------------------
mutex::~mutex() {

  // unlock the locked mutex if it was locked
  if ( _return_code == 0x0 ) pthread_mutex_unlock( &mutex::the_mutex );

};  // end dtor
