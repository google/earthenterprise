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
 *
 * Mostly used for unit testing because of an issue with C++11 and Google Test
 * framework.  The next version of Google Test will address the issue but even
 * then the printing might be nicer to read with these routines then the auto
 * generated binary prints Google test does for types that don't overload stream
 * operators for printing.
 */

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_VERREF_STORAGE_IO_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_VERREF_STORAGE_IO_H_

#include <iostream>
#include "common/verref_storage.h"

std::ostream & operator << (std::ostream &out, const _VerRefDef &vref_def)
{
    out << vref_def.GetRef();

    return out;
}

std::istream & operator >> (std::istream &in,  _VerRefDef &vref_def)
{
    std::string tmp_ref_str;
    in >> tmp_ref_str;
    _VerRefDef tmp_vref(tmp_ref_str);
    vref_def = tmp_vref;

    return in;
}

template<class _Tp, class _Ptr, class _Ref>
std::ostream & operator << (std::ostream &out, const _VerRefGenIterator<_Tp, _Ptr, _Ref> &it)
{
    out << it.cur_;

    return out;
}

template<class _Tp, class _Ptr, class _Ref>
std::istream & operator >> (std::istream &in,  _VerRefGenIterator<_Tp, _Ptr, _Ref> &it)
{
    in >> it.cur_;

    return in;
}

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_VERREF_STORAGE_IO_H_
