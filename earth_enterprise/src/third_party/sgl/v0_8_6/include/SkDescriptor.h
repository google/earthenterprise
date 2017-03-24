/* include/graphics/SkDescriptor.h
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

#ifndef SkDescriptor_DEFINED
#define SkDescriptor_DEFINED

#include "SkNoncopyable.h"

class SkDescriptor : SkNoncopyable {
public:
    static size_t ComputeOverhead(int entryCount)
    {
        SkASSERT(entryCount >= 0);
        return sizeof(SkDescriptor) + entryCount * sizeof(Entry);
    }

    static SkDescriptor* Alloc(size_t length)
    {
        SkASSERT(SkAlign4(length) == length);
        SkDescriptor* desc = (SkDescriptor*)sk_malloc_throw(length);
        return desc;
    }

    static void Free(SkDescriptor* desc)
    {
        sk_free(desc);
    }

    void init()
    {
        fLength = sizeof(SkDescriptor);
        fCount  = 0;
    }

    uint32_t getLength() const { return fLength; }

    void* addEntry(uint32_t tag, uint32_t length, const void* data = NULL)
    {
        SkASSERT(tag);
        SkASSERT(SkAlign4(length) == length);
        SkASSERT(this->findEntry(tag, NULL) == NULL);

        Entry*  entry = (Entry*)((char*)this + fLength);
        entry->fTag = tag;
        entry->fLen = length;
        if (data)
            memcpy(entry + 1, data, length);

        fCount += 1;
        fLength += sizeof(Entry) + length;
        return (entry + 1); // return its data
    }

    void computeChecksum()
    {
        fChecksum = SkDescriptor::ComputeChecksum(this);
    }

#ifdef SK_DEBUG
    void assertChecksum() const
    {
        SkASSERT(fChecksum == SkDescriptor::ComputeChecksum(this));
    }
#endif

    const void* findEntry(uint32_t tag, uint32_t* length) const
    {
        const Entry* entry = (const Entry*)(this + 1);
        int          count = fCount;

        while (--count >= 0)
        {
            if (entry->fTag == tag)
            {
                if (length)
                    *length = entry->fLen;
                return entry + 1;
            }
            entry = (const Entry*)((const char*)(entry + 1) + entry->fLen);
        }
        return NULL;
    }

    SkDescriptor* copy() const
    {
        SkDescriptor* desc = SkDescriptor::Alloc(fLength);
        memcpy(desc, this, fLength);
        return desc;
    }

    friend bool operator==(const SkDescriptor& a, const SkDescriptor& b)
    {
        return  a.fChecksum == b.fChecksum &&
                a.fLength == b.fLength &&
                // this assumes that fCount is the beginning of the rest of the descriptor
                // (after fCheckSum and fLength)
                memcmp(&a.fCount, &b.fCount, a.fLength - 2*sizeof(uint32_t)) == 0;
    }

    struct Entry {
        uint32_t fTag;
        uint32_t fLen;
    };

#ifdef SK_DEBUG
    uint32_t getChecksum() const { return fChecksum; }
    uint32_t getCount() const { return fCount; }
#endif

private:
    uint32_t fChecksum;  // must be first
    uint32_t fLength;    // must be second
    uint32_t fCount;

    static uint32_t ComputeChecksum(const SkDescriptor* desc)
    {
        const uint32_t*  ptr = (const uint32_t*)desc + 1; // skip the checksum field
        const uint32_t*  stop = (const uint32_t*)((const char*)desc + desc->fLength);
        uint32_t         sum = 0;

        SkASSERT(ptr < stop);
        do {
            sum = (sum << 1) | (sum >> 31);
            sum += *ptr++;
        } while (ptr < stop);

        return sum;
    }
    
    // private so no one can create one except our factories
    SkDescriptor() {}
};

#include "SkScalerContext.h"

class SkAutoDescriptor : SkNoncopyable {
public:
    SkAutoDescriptor(size_t size)
    {
        if (size <= sizeof(fStorage))
            fDesc = (SkDescriptor*)(void*)fStorage;
        else
            fDesc = SkDescriptor::Alloc(size);
    }
    ~SkAutoDescriptor()
    {
        if (fDesc != (SkDescriptor*)(void*)fStorage)
            SkDescriptor::Free(fDesc);
    }
    SkDescriptor* getDesc() const { return fDesc; }
private:
    enum {
        kStorageSize =  sizeof(SkDescriptor)
                        + sizeof(SkDescriptor::Entry) + sizeof(SkScalerContext::Rec)    // for rec
                        + sizeof(SkDescriptor::Entry) + sizeof(void*)                   // for typeface
                        + 32   // slop for occational small extras
    };
    SkDescriptor*   fDesc;
    uint32_t        fStorage[(kStorageSize + 3) >> 2];
};


#endif

