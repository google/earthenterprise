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


#ifndef _khcpp11_h_
#define _khcpp11_h_

// This will help with transitioning from C++98 to C++11.  Once fully transitioned over
// we should stop using this file and delete this file

#if __cplusplus <= 199711L    // C++98
// from Scott Meyers Effective C++:
// https://www.amazon.com/gp/product/0321334876?ie=UTF8&tag=aristeia.com-20&linkCode=as2&camp=1789&creative=9325&creativeASIN=0321334876
const                         /* this is a const object...     */
class nullptr_t
{
public:
   template<class T>          /* convertible to any type       */
   operator T*() const        /* of null non-member            */
      { return 0; }           /* pointer...                    */

   template<class C, class T> /* or any type of null           */
      operator T C::*() const /* member pointer...             */
      { return 0; }   

private:
   void operator&() const;    /* Can't take address of nullptr */

} nullptrx = {};               /* and whose name is nullptr     */
#  define nullptr nullptrx  // needed to avoid compiler error because nullptr
                            // is a C++11 keyword... But we don't use this if 
                            // C++11 or above is being used.
#else  // C++11 or greater
#  define GEE_HAS_MOVE 1
#  define GEE_HAS_STATIC_ASSERT 1
#endif // __cplusplus

#endif // _khcpp11_h_
