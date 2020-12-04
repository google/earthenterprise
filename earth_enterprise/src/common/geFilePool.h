/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef __geFilePool_h
#define __geFilePool_h

#include "geFilePoolImpl.h"
#include <khLRUMap.h>
#include "common/base/macros.h"

class EndianWriteBuffer;
class EndianReadBuffer;


// ****************************************************************************
// ***  geFilePool
// ***
// ***  This class is mostly thread safe - read on for more deatils
// ***
// ***  Manages a pool of file descriptors so the calling application
// ***  doesn't need to worry about how many fds it uses. To use it create a
// ***  geFilePool::Reader or a geFilePool::Writer object and call their
// ***  routines.
// ***
// ***  Readers are MT safe, and backing file descriptors come and go as needed
// ***  Additionally, multiple readers can be safely created for the same file.
// ***
// ***  Writers are only partially MT safe.  They lock their file descriptors
// ***  and must be closed before being destroyed. Only one thread can close
// ***  the Writer. Multiple threads can call Pwrite at the same time, so
// ***  long as they don't write to the same byte range on disk. Only one
// ***  Writer should be created per disk file.
// ***
// ***  Readers and writers should not be opened for the same file at the same
// ***  time. This is enforced with runtime checks and exceptions.
// ***
// ***  No background threads are involved. Calling threads block until
// ***  their IO has completed. If two threads are reading from the same
// ***  file, both will be allowed to enter pread simultaneously.
// ***
// ***  Typically you would make one file pool for the entire application
// ***  and pass it around to those places that need it.
// ***
// ***
// ***  --- main.cpp ---
// ***  geFilePool theFilePool;
// ***
// ***  --- other.cpp ---
// ***  geFilePool::Reader reader(theFilePool, filename);
// ***  reader.Pread(...);
// ***  reader.PRead(...);
// ****************************************************************************
class geFilePool {
 public:
  class Reader {
   public:
    Reader(geFilePool &pool_, const std::string &fname_);
    ~Reader(void);
    std::uint64_t Filesize(void) const;

    // will throw if unable to read all bytes
    void Pread(void *buffer, size_t size, off64_t offset);

    // will resize buffer if necessary, will throw if unable to read all bytes
    void Pread(std::string &buffer, size_t size, off64_t offset);

    // Like above, but interprets last 4 bytes of data read as the CRC
    // of all the preceeding bytes. Checks the CRC and throws if it doesn't
    // match. Returned buffer still contains the trailing CRC.
    void PreadCRC(void *buffer, size_t size, off64_t offset);
    void PreadCRC(std::string &buffer, size_t size, off64_t offset);

   private:
    LockingFileReference fileref;
    DISALLOW_COPY_AND_ASSIGN(Reader);
  };

  class Writer {
   public:
    enum WriteStyle { WriteOnly = O_WRONLY, ReadWrite = O_RDWR };
    enum TruncateStyle { NoTruncate = 0, Truncate = O_TRUNC };

    Writer(WriteStyle wStyle, TruncateStyle tStyle,
           geFilePool &pool_, const std::string &fname_,
           mode_t createMask = 0666);

    // SyncAndClose must be called before the Writer is destroyed
    // This is because a close can fail to write the remaining cached bytes
    // to disk. We want to throw an exception in that case, but we're not
    // supposed to throw exceptions from destructors.
    ~Writer(void) noexcept(false);

    // Buffer contiguous writes with the given size buffer. write_buffer_size is
    // in bytes. 0 turns off buffering (default). Safe but not recommended for
    // multi-threaded writing.
    void BufferWrites(size_t write_buffer_size);

    // these will throw on any error
    void Pwrite(const void *buffer, size_t size, off64_t offset);
    // CRC versions of will overwrite last 4 bytes in buffer with CRC of
    // previous bytes.  Buffer should be 4 bytes larger than actual
    // data. size includes the CRC
    void PwriteCRC(void *buffer, size_t size, off64_t offset);
    void SyncAndClose(void);

    // See comments in reader above
    void Pread(void *buffer, size_t size, off64_t offset);
    void Pread(std::string &buffer, size_t size, off64_t offset);
    void PreadCRC(void *buffer, size_t size, off64_t offset);
    void PreadCRC(std::string &buffer, size_t size, off64_t offset);

   private:
    // Flush the write buffer, assuming not locked.
    void FlushWriteBuffer();
    // Flush the write buffer, assuming already locked.
    void FlushWriteBufferLocked();
    // Variables for the write buffer.
    khMutex wb_lock_;           // Lock to protect buffer for multiple threads
    size_t wb_size_;            // Size of total write buffer
    khDeleteGuard<char, ArrayDeleter> write_buffer_;  // The buffer
    off64_t wb_buffer_offset_;  // Buffer offset in file
    size_t wb_length_;          // Number of bytes currently in buffer

    LockingFileReference fileref;
    DISALLOW_COPY_AND_ASSIGN(Writer);
  };

  // whole file convenience routines, buffers must be appropriately sized
  void WriteSimpleFile(const std::string &fname,
                       const void* buf,
                       size_t size,
                       mode_t createMask = 0666);
  void ReadSimpleFile(const std::string &fname, void* buf, size_t size);

  // std::string variants of the above
  void WriteStringFile(const std::string &fname,
                       const std::string &buf,
                       mode_t createMask = 0666);
  void ReadStringFile(const std::string &fname, std::string *buf);

  // CRC variants - NOTE: these differ from the lower level routines
  // in the fact that the CRC is hidden from the caller, space isn't require
  // to be reserved and the returned buffer doesn't have the CRC included
  void WriteSimpleFileWithCRC(const std::string &fname,
                              EndianWriteBuffer &buf,
                              mode_t createMask = 0666);
  void ReadSimpleFileWithCRC(const std::string &fname,
                             EndianReadBuffer &buf);



  // negative -> RLIMIT_NOFILE - specified
  // 0 -> assertion
  geFilePool(int maxNumFds_ = -50);

  // Will assert if there are any open readers or writers
  // will close all open fds - but will ignore the return value from ::close
  ~geFilePool(void);

  // Non thread safe version for testing purposes.
  unsigned int MaxFdsUsed(void) const {  return maxFdsUsed; }

  // For debugging purposes
  void DumpState(bool print_all_filerefs);

 private:
  friend class Reader;
  friend class Writer;
  friend class SignalingFileReservation;
  friend class FileReservationImpl;
  friend class FileReferenceImpl;
  friend class LockingFileReference;

  const unsigned int maxNumFds;
  unsigned int numFdsUsed;
  mutable unsigned int maxFdsUsed;
  mutable khMutex mutex;
  khCondVar condvar;

  LockingFileReference GetFileReference(const std::string &fname,
                                        int createFlags, mode_t createMask);

  // mutex must already be locked before calling these routines
  inline void NotifyWaiters_locked(void) { condvar.broadcast_all(); }
  inline void WaitForChanges_locked(void) { condvar.wait(mutex); }
  inline bool AllReservationsInUse_locked(void) {
    return numFdsUsed >= maxNumFds;
  }
  bool FindOldestUnusedReservation_locked(FileReference *found);
  void ReduceFdCount_locked(void);
  void IncreaseFdCount_locked(void);

  // stuff for registration of readers & writers
  typedef khLRUMap<std::string, FileReferenceImpl*> FileMap;
  FileMap filerefs;

  DISALLOW_COPY_AND_ASSIGN(geFilePool);
};

#endif /* __geFilePool_h */
