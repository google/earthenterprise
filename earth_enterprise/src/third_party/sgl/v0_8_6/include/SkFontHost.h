/* include/graphics/SkFontHost.h
**
** Copyright 2006, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef SkFontHost_DEFINED
#define SkFontHost_DEFINED

#include "SkScalerContext.h"
#include "SkTypeface.h"

class SkDescriptor;
class SkStream;

/** \class SkFontHost

    This class is ported to each environment. It is responsible for bridging the gap
    between SkTypeface and the resulting platform-specific instance of SkScalerContext.
*/
class SkFontHost {
public:
    /** Return a subclass of SkTypeface, one that can be used by your scalaracontext
        (returned by SkFontHost::CreateScalarContext).
        1) If family is null, use name.
        2) If name is null, use family.
        3) If both are null, use default family.
    */
    static SkTypeface* CreateTypeface(const SkTypeface* family, const char name[], SkTypeface::Style);
    /** Given a typeface (or null), return the number of bytes needed to flatten it
        into a buffer, for the purpose of communicating information to the
        scalercontext. If buffer is null, then ignore it but still return the number
        of bytes that would be written.
    */
    static uint32_t FlattenTypeface(const SkTypeface* face, void* buffer);
    /** Return a string given a descriptor. This is used as a key for looking up in a cache,
        to see if we have already opened the underlying font for this typeface. The string
        may also be used for debugging output, but it is not a "user" string, so it need
        not be localized or otherwise masssaged. The string should only depend on the underlying
        font data, and not on any size info.
    */
    static void GetDescriptorKeyString(const SkDescriptor* desc, SkString* keyString);
    /** Given a descriptor and its corresponding keyString (returned by GetTypefaceKeyString),
        return a new stream for read-only access to the font data.
    */
    static SkStream* OpenDescriptorStream(const SkDescriptor* desc, const char keyString[]);

    /** Return a subclass of SkScalarContext
    */
    static SkScalerContext* CreateScalerContext(const SkDescriptor* desc);
    /** Return a scalercontext using the "fallback" font. If there is no designated
        fallback, return null.
    */
    static SkScalerContext* CreateFallbackScalerContext(const SkScalerContext::Rec&);

    /** Return a 32bit hash key for the given typeface. If face is null, return the hash
        for the default typeface.
    */
    static uint32_t TypefaceHash(const SkTypeface* face);

    /** Return true if a and b represent the same typeface. Note this can be true
        even if one of them are null, as long as the other is also null or represents
        the "default" typeface.
    */
    static bool TypefaceEqual(const SkTypeface* a, const SkTypeface* b);
};

#endif

