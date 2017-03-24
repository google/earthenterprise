/* include/graphics/SkStream.h
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

#ifndef SkStream_DEFINED
#define SkStream_DEFINED

#include "SkNoncopyable.h"
#include "SkScalar.h"

class SkStream : SkNoncopyable {
public:
    virtual ~SkStream();
    /** Called to rewind to the beginning of the stream. If this cannot be
        done, return false.
    */
    virtual bool rewind() = 0;
    /** If this stream represents a file, this method returns the file's name.
        If it does not, it returns NULL (the default behavior).
    */
    virtual const char* getFileName();
    /** Called to read or skip size number of bytes.
        If buffer is NULL and size > 0, skip that many bytes, returning how many were skipped.
        If buffer is NULL and size == 0, return the total length of the stream.
        If buffer != NULL, copy the requested number of bytes into buffer, returning how many were copied.
        @param buffer   If buffer is NULL, ignore and just skip size bytes, otherwise copy size bytes into buffer
        @param size The number of bytes to skip or copy
        @return bytes read on success
    */
    virtual size_t read(void* buffer, size_t size) = 0;

    /** Return the total length of the stream.
    */
    size_t getLength() { return this->read(NULL, 0); }
    
    /** Skip the specified number of bytes, returning the actual number
        of bytes that could be skipped.
    */
    size_t skip(size_t bytes);
            

    /** If the stream is backed by RAM, this method returns the starting
        address for the data. If not (i.e. it is backed by a file or other
        structure), this method returns NULL.
        The default implementation returns NULL.
    */
    virtual const void* getMemoryBase();

    static SkStream* GetURIStream(const char prefix[], const char path[]);
    static bool IsAbsoluteURI(const char path[]);
};

class SkWStream : SkNoncopyable {
public:
    virtual ~SkWStream();

    /** Called to write bytes to a SkWStream. Returns true on success
        @param buffer the address of at least size bytes to be written to the stream
        @param size The number of bytes in buffer to write to the stream
        @return true on success
    */
    virtual bool write(const void* buffer, size_t size) = 0;
    virtual void newline();
    virtual void flush();

    // helpers

    bool    writeText(const char text[]);
    bool    writeDecAsText(int32_t);
    bool    writeHexAsText(uint32_t, int minDigits = 0);
    bool    writeScalarAsText(SkScalar);

    SkDEBUGCODE(static void UnitTest();)
};

////////////////////////////////////////////////////////////////////////////////////////

#include "SkString.h"

struct SkFILE;

class SkFILEStream : public SkStream {
public:
    SkFILEStream(const char path[] = NULL);
    virtual ~SkFILEStream();

    /** Returns true if the current path could be opened.
    */
    bool isValid() const { return fFILE != NULL; }
    /** Close the current file, and open a new file with the specified
        path. If path is NULL, just close the current file.
    */
    void setPath(const char path[]);
    
    SkFILE* getSkFILE() const { return fFILE; }

    virtual bool rewind();
    virtual size_t read(void* buffer, size_t size);
    virtual const char* getFileName();

private:
    SkFILE*     fFILE;
    SkString    fName;
};

class SkMemoryStream : public SkStream {
public:
    SkMemoryStream();
    SkMemoryStream(const void* data, size_t length);

    /** Resets the stream to the specified data and length,
        just like the constructor.
    */
    virtual void setMemory(const void* data, size_t length);

    virtual bool rewind();
    virtual size_t read(void* buffer, size_t size);
    virtual const void* getMemoryBase();

private:
    const void* fSrc;
    size_t fSize, fOffset;
};

/** \class SkBufferStream
    This is a wrapper class that adds buffering to another stream.
    The caller can provide the buffer, or ask SkBufferStream to allocated/free
    it automatically.
*/
class SkBufferStream : public SkStream {
public:
    /** Provide the stream to be buffered (proxy), and the size of the buffer that
        should be used. This will be allocated and freed automatically. If bufferSize is 0,
        a default buffer size will be used.
    */
    SkBufferStream(SkStream& proxy, size_t bufferSize = 0);
    /** Provide the stream to be buffered (proxy), and a buffer and size to be used.
        This buffer is owned by the caller, and must be at least bufferSize bytes big.
        Passing NULL for buffer will cause the buffer to be allocated/freed automatically.
        If buffer is not NULL, it is an error for bufferSize to be 0.
    */
    SkBufferStream(SkStream& proxy, void* buffer, size_t bufferSize);
    virtual ~SkBufferStream();

    virtual bool        rewind();
    virtual const char* getFileName();
    virtual size_t      read(void* buffer, size_t size);
    virtual const void* getMemoryBase();

private:
    enum {
        kDefaultBufferSize  = 128
    };
    // illegal
    SkBufferStream(const SkBufferStream&);
    SkBufferStream& operator=(const SkBufferStream&);

    SkStream&   fProxy;
    char*       fBuffer;
    size_t      fOrigBufferSize, fBufferSize, fBufferOffset;
    bool        fWeOwnTheBuffer;

    void    init(void*, size_t);
};

/////////////////////////////////////////////////////////////////////////////////////////////

class SkFILEWStream : public SkWStream {
public:
            SkFILEWStream(const char path[]);
    virtual ~SkFILEWStream();

    /** Returns true if the current path could be opened.
    */
    bool isValid() const { return fFILE != NULL; }

    virtual bool write(const void* buffer, size_t size);
    virtual void flush();
private:
    SkFILE* fFILE;
};

class SkMemoryWStream : public SkWStream {
public:
    SkMemoryWStream(void* buffer, size_t size);
    virtual bool write(const void* buffer, size_t size);
    
private:
    char*   fBuffer;
    size_t  fMaxLength;
    size_t  fBytesWritten;
};

class SkDynamicMemoryWStream : public SkWStream {
public:
    SkDynamicMemoryWStream();
    virtual ~SkDynamicMemoryWStream();
    virtual bool write(const void* buffer, size_t size);
    // random access write
    // modifies stream and returns true if offset + size is less than or equal to getOffset()
    bool write(const void* buffer, size_t offset, size_t size); 
    size_t getOffset() { return fBytesWritten; }

    // copy what has been written to the stream into dst
    void    copyTo(void* dst) const;
    /*  return a cache of the flattened data returned by copyTo().
        This copy is only valid until the next call to write().
        The memory is managed by the stream class.
    */
    const char* getStream() const;

private:
    struct Block;
    Block*  fHead;
    Block*  fTail;
    size_t  fBytesWritten;
    mutable char*   fCopyToCache;
};


class SkDebugWStream : public SkWStream {
public:
    // overrides
    virtual bool write(const void* buffer, size_t size);
    virtual void newline();
};

// for now
typedef SkFILEStream SkURLStream;

#endif

