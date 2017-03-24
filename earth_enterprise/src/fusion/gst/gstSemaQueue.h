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

#ifndef _gstSemaQueue_h_
#define _gstSemaQueue_h_

#ifndef DOXYGEN_SKIP
#include <pthread.h>
#include <semaphore.h>
#include <gstQueue.h>
#include <gstTypes.h>
#endif // DOXYGEN_SKIP

/*!
 * \file
 * \brief Semaphore and lock protected Queue
 * \author Philip Nemec
 * \version \verbatim $Revision: 3619 $ \endverbatim
 * \date \verbatim $DateTime: 2001/04/10 18:13:49 $ \endverbatim
 */

/*!
 * \brief Semaphore and lock protected Queue
 *
 * Lock to make thread safe,
 * semaphore so get blocks until something available to get.
 */

template <class Type>
class gstSemaQueue : public gstArrayQueue<Type> {
 public:
  gstSemaQueue(int sz) : gstArrayQueue<Type>(sz)
  {
    pthread_mutex_init( &elements_lock, NULL );
    sem_init( &empty_sema, FALSE, 0 );
    sem_init( &full_sema, FALSE, sz );
  }

  ~gstSemaQueue()
  {
    pthread_mutex_destroy( &elements_lock );
    sem_destroy( &empty_sema );
    sem_destroy( &full_sema );
  }

  void put( Type item )
  {
    sem_wait( &full_sema );     // wait here until non-empty
    pthread_mutex_lock( &elements_lock );
    gstArrayQueue<Type>::put( item );
    pthread_mutex_unlock( &elements_lock );
    sem_post( &empty_sema );
  }

  Type get()
  {
    sem_wait( &empty_sema );
    pthread_mutex_lock( &elements_lock );
    Type ret = gstArrayQueue<Type>::get();
    pthread_mutex_unlock( &elements_lock );
    sem_post( &full_sema );
    return ret;
  }

  void clear()
  {
    pthread_mutex_lock( &elements_lock );
    gstArrayQueue<Type>::clear();
    pthread_mutex_unlock( &elements_lock );
  }

  int size()
  {
    pthread_mutex_lock( &elements_lock );
    int ret = gstArrayQueue<Type>::size();
    pthread_mutex_unlock( &elements_lock );
    return ret;
  }

  // only use this when exiting threads that are blocked
  // on the above put(), get(), clear(), size()
  void cancel()
  {
    sem_post( &full_sema );
    sem_post( &empty_sema );
  }

 private:
  pthread_mutex_t elements_lock; // protect access to elements
  sem_t empty_sema; // block get (if empty) until not empty
  sem_t full_sema; // block put (if full) until not full
};

#endif // !_gstFIFO_h_
