/* libs/graphics/sgl/SkDeque.cpp
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

#include "SkDeque.h"

#define INIT_ELEM_COUNT 1  // should we let the caller set this in the constructor?

struct SkDeque::Head {
    Head*   fNext;
    Head*   fPrev;
    char*   fBegin; // start of used section in this chunk
    char*   fEnd;   // end of used section in this chunk
    char*   fStop;  // end of the allocated chunk
    
    char*       start() { return (char*)(this + 1); }
    const char* start() const { return (const char*)(this + 1); }
    
    void init(size_t size)
    {
        fNext   = fPrev = NULL;
        fBegin  = fEnd = NULL;
        fStop   = (char*)this + size;
    }
};

SkDeque::SkDeque(size_t elemSize) : fElemSize(elemSize), fInitialStorage(NULL), fCount(0)
{
    fFront = fBack = NULL;
}

SkDeque::SkDeque(size_t elemSize, void* storage, size_t storageSize)
    : fElemSize(elemSize), fInitialStorage(storage), fCount(0)
{
    SkASSERT(storageSize == 0 || storage != NULL);
    
    if (storageSize >= sizeof(Head) + elemSize)
    {
        fFront = (Head*)storage;
        fFront->init(storageSize);
    }
    else
    {
        fFront = NULL;
    }
    fBack = fFront;
}

SkDeque::~SkDeque()
{
    Head* head = fFront;
    Head* initialHead = (Head*)fInitialStorage;

    while (head)
    {
        Head* next = head->fNext;
        if (head != initialHead)
            sk_free(head);
        head = next;
    }
}

const void* SkDeque::front() const
{
    Head* front = fFront;
    
    if (front == NULL)
        return NULL;
    
    if (front->fBegin == NULL)
    {
        front = front->fNext;
        if (front == NULL)
            return NULL;
    }
    SkASSERT(front->fBegin);
    return front->fBegin;
}

const void* SkDeque::back() const
{
    Head* back = fBack;

    if (back == NULL)
        return NULL;

    if (back->fEnd == NULL)  // marked as deleted
    {
        back = back->fPrev;
        if (back == NULL)
            return NULL;
    }
    SkASSERT(back->fEnd);
    return back->fEnd - fElemSize;
}

void* SkDeque::push_front()
{
    fCount += 1;

    if (fFront == NULL)
    {
        fFront = (Head*)sk_malloc_throw(sizeof(Head) + INIT_ELEM_COUNT * fElemSize);
        fFront->init(sizeof(Head) + INIT_ELEM_COUNT * fElemSize);
        fBack = fFront;     // update our linklist
    }
    
    Head*   first = fFront;
    char*   begin;

    if (first->fBegin == NULL)
    {
    INIT_CHUNK:
        first->fEnd = first->fStop;
        begin = first->fStop - fElemSize;
    }
    else
    {
        begin = first->fBegin - fElemSize;
        if (begin < first->start())     // no more room in this chunk
        {
            // should we alloc more as we accumulate more elements?
            size_t  size = sizeof(Head) + INIT_ELEM_COUNT * fElemSize;

            first = (Head*)sk_malloc_throw(size);
            first->init(size);
            first->fNext = fFront;
            fFront->fPrev = first;
            fFront = first;
            goto INIT_CHUNK;
        }
    }

    first->fBegin = begin;
    return begin;
}

void* SkDeque::push_back()
{
    fCount += 1;

    if (fBack == NULL)
    {
        fBack = (Head*)sk_malloc_throw(sizeof(Head) + INIT_ELEM_COUNT * fElemSize);
        fBack->init(sizeof(Head) + INIT_ELEM_COUNT * fElemSize);
        fFront = fBack; // update our linklist
    }
    
    Head*   last = fBack;
    char*   end;

    if (last->fBegin == NULL)
    {
    INIT_CHUNK:
        last->fBegin = last->start();
        end = last->fBegin + fElemSize;
    }
    else
    {
        end = last->fEnd + fElemSize;
        if (end > last->fStop)  // no more room in this chunk
        {
            // should we alloc more as we accumulate more elements?
            size_t  size = sizeof(Head) + INIT_ELEM_COUNT * fElemSize;

            last = (Head*)sk_malloc_throw(size);
            last->init(size);
            last->fPrev = fBack;
            fBack->fNext = last;
            fBack = last;
            goto INIT_CHUNK;
        }
    }

    last->fEnd = end;
    return end - fElemSize;
}

void SkDeque::pop_front()
{
    SkASSERT(fCount > 0);
    fCount -= 1;

    Head*   first = fFront;

    SkASSERT(first != NULL);
    
    if (first->fBegin == NULL)   // we were marked empty from before
    {
        first = first->fNext;
        first->fPrev = NULL;
        sk_free(fFront);
        fFront = first;
        SkASSERT(first != NULL);    // else we popped too far
    }

    char* begin = first->fBegin + fElemSize;
    SkASSERT(begin <= first->fEnd);

    if (begin < fFront->fEnd)
        first->fBegin = begin;
    else
        first->fBegin = first->fEnd = NULL;  // mark as empty
}

void SkDeque::pop_back()
{
    SkASSERT(fCount > 0);
    fCount -= 1;

    Head* last = fBack;
    
    SkASSERT(last != NULL);
    
    if (last->fEnd == NULL)  // we were marked empty from before
    {
        last = last->fPrev;
        last->fNext = NULL;
        sk_free(fBack);
        fBack = last;
        SkASSERT(last != NULL);  // else we popped too far
    }
    
    char* end = last->fEnd - fElemSize;
    SkASSERT(end >= last->fBegin);

    if (end > last->fBegin)
        last->fEnd = end;
    else
        last->fBegin = last->fEnd = NULL;    // mark as empty
}

///////////////////////////////////////////////////////////////////////////////

SkDeque::Iter::Iter(const SkDeque& d) : fElemSize(d.fElemSize)
{
    fHead = d.fFront;
    while (fHead != NULL && fHead->fBegin == NULL)
        fHead = fHead->fNext;
    fPos = fHead ? fHead->fBegin : NULL;
}

void* SkDeque::Iter::next()
{
    char* pos = fPos;
    
    if (pos)   // if we were valid, try to move to the next setting
    {
        char* next = pos + fElemSize;
        SkASSERT(next <= fHead->fEnd);
        if (next == fHead->fEnd) // exhausted this chunk, move to next
        {
            do {
                fHead = fHead->fNext;
            } while (fHead != NULL && fHead->fBegin == NULL);
            next = fHead ? fHead->fBegin : NULL;
        }
        fPos = next;
    }
    return pos;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef SK_DEBUG

#include <new>

static size_t gTestClassCount;

//#define SPEW_LIFETIME

class TestClass {
public:
    TestClass()
    {
        ++gTestClassCount;
#ifdef SPEW_LIFETIME
        SkDebugf("gTestClassCount=%d\n", gTestClassCount);
#endif
    }
    TestClass(int value) : fValue(value)
    {
        ++gTestClassCount;
#ifdef SPEW_LIFETIME
        SkDebugf("gTestClassCount=%d\n", gTestClassCount);
#endif
    }
    TestClass(const TestClass& src) : fValue(src.fValue)
    {
        ++gTestClassCount;
#ifdef SPEW_LIFETIME
        SkDebugf("gTestClassCount=%d\n", gTestClassCount);
#endif
    }
    ~TestClass()
    {
        --gTestClassCount;
#ifdef SPEW_LIFETIME
        SkDebugf("~gTestClassCount=%d\n", gTestClassCount);
#endif
    }
    int fValue;
};

SK_SET_TYPE_TRAITS(TestClass, false, false, false, true);

void SkDeque::UnitTest()
{
    {
        SkTDeque<int>           d1;
        SkTDeque<int>           d2;
        SkTDeque<TestClass>     d3;
        SkSTDeque<5, TestClass> d4;

        int i;
        
        SkASSERT(d1.empty());
        SkASSERT(d2.empty());
        SkASSERT(d3.empty());
        SkASSERT(d4.empty());

        for (i = 0; i < 100; i++)
        {
            d1.push_front(i);
            SkASSERT(*d1.front() == i);
            SkASSERT(*d1.back() == 0);

            d2.push_back(i);
            SkASSERT(*d2.back() == i);
            SkASSERT(*d2.front() == 0);

            d3.push_front(TestClass(i));
            d3.push_back(TestClass(-i));
            SkASSERT(d3.front()->fValue == i);
            SkASSERT(d3.back()->fValue == -i);

            d4.push_front()->fValue = i;
            d4.push_back()->fValue = -i;
            SkASSERT(d4.front()->fValue == i);
            SkASSERT(d4.back()->fValue == -i);
        }
        
        SkASSERT(d1.count() == 100);
        SkASSERT(d2.count() == 100);
        SkASSERT(d3.count() == 200);
        SkASSERT(d4.count() == 200);
        
        {
            SkTDeque<int>::Iter iter(d2);
            int*                curr;
            
            i = 0;
            while ((curr = iter.next()) != NULL) {
                SkASSERT(*curr == i);
                i += 1;
            }
            SkASSERT(i == 100);
        }
        
        for (i = 0; i < 50; i++)
        {
            d1.pop_front(); d1.pop_back();
            d2.pop_front(); d2.pop_back();
            d3.pop_front(); d3.pop_back();
            d4.pop_front(); d4.pop_back();
        }

        SkASSERT(d1.count() == 0);
        SkASSERT(d2.count() == 0);
        SkASSERT(d3.count() == 100);
        SkASSERT(d4.count() == 100);
    }
    int counter = gTestClassCount;
    SkASSERT(counter == 0);
}

#endif

