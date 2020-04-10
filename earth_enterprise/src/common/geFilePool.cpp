// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include "geFilePool.h"
#include <khFileUtils.h>
#include <khSimpleException.h>
#include <third_party/rsa_md5/crc32.h>
#include <khEndian.h>

// ****************************************************************************
// ***  FileReservationImpl
// ****************************************************************************
bool FileReservationImpl::UnlockAndClose_(geFilePool &pool) {
  // run when pool.mutex IS locked
  if (fd != -1) {
    int result = 0;
    {
      khUnlockGuard unlock(pool.mutex);
      result = isWriter ? khFsyncAndClose(fd) : khClose(fd);
      fd = -1;
    }
    pool.ReduceFdCount_locked();
    return (result != -1);
  } else {
    return true;
  }
}

bool FileReservationImpl::UnlockAndOpen_(geFilePool &pool,
                                         const std::string &fname, int flags,
                                         mode_t createMask)
{
  // run when pool.mutex IS locked
  pool.IncreaseFdCount_locked(); // Acquire the FD count before unlocking!!!
                                 // Don't give another thread a chance to break
                                 // our invariant numFdsUsed_ < maxNumFds
  {
    khUnlockGuard unlock(pool.mutex);
    fd = khOpen(fname, flags, createMask);
  }
  if (fd < 0) {
    notify(NFY_DEBUG, "FileReservationImpl::UnlockAndOpen_ failure: "
           "%u file: %s flags: %d mask: %x, errno: %d\n",
           pool.MaxFdsUsed(), fname.c_str(),flags, createMask, errno);
    // If we failed, back off the count.
    pool.ReduceFdCount_locked();
  }
  return (fd != -1);
}

// ****************************************************************************
// ***  SignalingFileReservation
// ****************************************************************************
SignalingFileReservation::SignalingFileReservation(geFilePool &pool_,
                                                   const FileReservation &o)
    : FileReservation(o), pool(pool_)
{ }

SignalingFileReservation::~SignalingFileReservation(void) {
  // run when pool.mutex is NOT locked

  // release my count now
  release();

  // tell parent that the reservation might be stealable
  {
    khLockGuard lock(pool.mutex);
    pool.NotifyWaiters_locked();
  }
}

// ****************************************************************************
// ***  FileReferenceImpl::ChangingGuard
// ****************************************************************************
FileReferenceImpl::ChangingGuard::ChangingGuard(FileReferenceImpl *file) :
    fileref(file) {
  fileref->SetOperationPending_locked();
}

FileReferenceImpl::ChangingGuard::~ChangingGuard(void) {
  fileref->ClearOperationPending_locked();
}


// ****************************************************************************
// ***  FileReferenceImpl
// ****************************************************************************
FileReferenceImpl::FileReferenceImpl(geFilePool &pool_,
                                     const std::string &fname_,
                                     int openFlags_,
                                     int createMask_) :
    pool(pool_), fname(fname_), reservation(), operationPending(false),
    closeError(0), openFlags(openFlags_),
    createMask(createMask_), cachedFilesize(0) {
  // run when pool.mutex IS locked

  if (!IsWriter()) {
    time_t mtime;
    if (!khGetFileInfo(fname, cachedFilesize, mtime)) {
      throw khSimpleException("No such file ") << fname;
    }
  }

  pool.filerefs.Add(fname, this);
}

void FileReferenceImpl::SetOperationPending_locked(void) {
  // run when pool.mutex IS locked
  operationPending = true;
}

void FileReferenceImpl::ClearOperationPending_locked(void) {
  // run when pool.mutex IS locked
  operationPending = false;
  pool.NotifyWaiters_locked();
}

FileReferenceImpl::~FileReferenceImpl(void) {
  // run when pool.mutex IS locked
  if (!std::uncaught_exception()) {
    assert(!(CareAboutCloseErrors() && reservation));
  }

  if (reservation) {
    // mark myself as changing so others have to wait
    ChangingGuard changeGuard(this);

    // release my claim on the reservation
    FileReservation tmpres = reservation;
    reservation.release();

    // close the fd (after unlocking)
    // we know from above that we don't care about the error
    (void) tmpres->UnlockAndClose_(pool);
  }

  // now that we've actually closed the file, take self out of filerefs map
  pool.filerefs.Remove(fname);
}

void FileReferenceImpl::ReleaseReservation_locked(void) {
  // run when pool.mutex IS locked
  assert(reservation);
  assert(reservation->refcount()==1);

  // mark myself as changing so others have to wait
  ChangingGuard changeGuard(this);

  // release my claim on the reservation
  FileReservation tmpres = reservation;
  reservation.release();

  // close the exiting fd (after unlocking)
  bool success = tmpres->UnlockAndClose_(pool);

  // save the close error if we care to
  if (!success && CareAboutCloseErrors()) {
    closeError = errno;
  }
}

void FileReferenceImpl::Open_locked(void) {
  // run when pool.mutex IS locked
  assert(reservation);
  assert(reservation->refcount()==1);

  // open the fd (after unlocking)
  bool success = reservation->UnlockAndOpen_(pool, fname, openFlags,
                                             createMask);
  // propagate the error
  if (!success) {
    reservation.release();
    throw khSimpleErrnoException("Error opening ") << fname;
  }
  // we've sucessfully opened. If user asked for O_TRUNC, clear it now. That
  // way if the fd gets reused and we have to open it again later we won't
  // truncate it again.
  openFlags = openFlags & ~O_TRUNC;
}

SignalingFileReservation FileReferenceImpl::GetReservation() {
  // run when pool.mutex is NOT locked
  khLockGuard lock(pool.mutex);

  // see if somebody elase already closed my file and left me an error to
  // deal with
  if (closeError) {
    assert(!reservation);
    errno = closeError;
    closeError = 0;
    throw khSimpleErrnoException("Error closing ") << fname;
  }

  while (1) {
    if (operationPending) {
      // Another thread is opening or closing this file
      // wait until something changes and try again
      pool.WaitForChanges_locked();
      continue;
    }

    // If I already have a reservation , just return it
    if (reservation) {
      return SignalingFileReservation(pool, reservation);
    }

    // Mark myself as being in the middle of the open (makes others wait)
    // This is beacause the "steal" operation below will cause me to
    // temporarily release the lock
    ChangingGuard changeGuard(this);

    while (pool.AllReservationsInUse_locked()) {

      FileReference closeFile;
      if (!pool.FindOldestUnusedReservation_locked(&closeFile)) {
        // All the reservations are busy and I'm not allowed to steal one
        // wait until something changes and try again
        pool.WaitForChanges_locked();
        continue;
      }

      // Close the reservation
      closeFile->ReleaseReservation_locked();
    }
    reservation = khRefGuardFromNew(new FileReservationImpl());

    // Basic check since this was broken in geFilePool implementation.
    if (pool.numFdsUsed >= pool.maxNumFds) {
      throw khSimpleException(
        "FileReferenceImpl::GetReservation: too many files open: ") <<
        "numFdsUsed: " << pool.numFdsUsed << " maxNumFds:" << pool.maxNumFds;
    }
    // Now open the file with my new reservation
    // Will temporarily release lock
    Open_locked();

    return SignalingFileReservation(pool, reservation);
  }
}
void FileReferenceImpl::Dump_locked(void) {
  // run when pool.mutex IS locked

  fprintf(stderr, "%s: refcount=%d ", fname.c_str(), refcount());
  if (reservation) {
    fprintf(stderr, "fd=%d ", reservation->Fd() );
  }
  if (operationPending) {
    fprintf(stderr, "pending ");
  }
  if (closeError) {
    fprintf(stderr, "errno=%d ", closeError);
  }
  fprintf(stderr, "\n");
}

void FileReferenceImpl::Pread(void *buffer, size_t size, off64_t offset) {
  // run when pool.mutex is NOT locked
  SignalingFileReservation tmpres(GetReservation());

  if (!tmpres) {
    throw khSimpleException("FileReferenceImpl::Pread: NULL tmpres");
  }

  if (!khPreadAll(tmpres->Fd(), buffer, size, offset)) {
    throw khSimpleErrnoException()
      << "Unable to read " << size << " bytes from offset "
      << offset << " in " << fname;
  }
}

void FileReferenceImpl::Pwrite(const void *buffer, size_t size,
                               off64_t offset) {
  // run when pool.mutex is NOT locked
  SignalingFileReservation tmpres(GetReservation());

  if (!tmpres) {
    throw khSimpleException("FileReferenceImpl::Pwrite: NULL tmpres");
  }

  if (!khPwriteAll(tmpres->Fd(), buffer, size, offset)) {
    throw khSimpleErrnoException()
      << "Unable to write " << size << " bytes to offset "
      << offset << " in " << fname;
  }
}

void FileReferenceImpl::Close(void) {
  // run when pool.mutex is NOT locked
  khLockGuard lock(pool.mutex);
  if (closeError) {
    assert(!reservation);
    errno = closeError;
    closeError = 0;
    throw khSimpleErrnoException("Error closing ") << fname;
  }
  if (reservation) {
    // mark myself as changing so others have to wait
    ChangingGuard changeGuard(this);

    // release my claim on the reservation
    FileReservation tmpres = reservation;
    reservation.release();

    // close the fd (after unlocking)
    bool success = tmpres->UnlockAndClose_(pool);
    if (!success) {
      throw khSimpleErrnoException("Error closing ") << fname;
    }
  }
}

// ****************************************************************************
// ***  geFilePool
// ****************************************************************************
namespace {
unsigned int CalcMaxFds(int requested) {
  assert(requested != 0);
  int systemMax = khMaxOpenFiles();
  if (requested > 0) {
    return std::min(systemMax, requested);
  } else {
    return std::max(1, systemMax+requested);
  }
}
}

geFilePool::geFilePool(int maxNumFds_) :
    maxNumFds(CalcMaxFds(maxNumFds_)),
    numFdsUsed(0), maxFdsUsed(0),
    mutex(), condvar()
{
  notify(NFY_DEBUG, "Creating geFilePool: maxNumFds: %d\n", maxNumFds);
}

geFilePool::~geFilePool(void) {
  // This had better be the old thread accessing this object now
  // because when I'm done it won't exist anymore

  if (!std::uncaught_exception()) {
    assert(filerefs.size() == 0);
  }
}

void geFilePool::DumpState(bool print_all_filerefs)
{
  // Don't bother locking this...thread safety here not an issue...better
  // to avoid deadlocks.
  // was:  khLockGuard lock(mutex);

  fprintf(stderr, "----- geFilePool -----\n");
  fprintf(stderr, "maxNumFds  = %u ", maxNumFds);
  fprintf(stderr, "maxFdsUsed = %u ", maxFdsUsed);
  fprintf(stderr, "numFdsUsed = %u ", numFdsUsed);
  fprintf(stderr, "file references:\n");
  if (print_all_filerefs) {
    filerefs.Apply(std::mem_fun_ref(&FileReferenceImpl::Dump_locked));
  }
}


void geFilePool::WriteSimpleFile(const std::string &fname,
                                 const void* buf, size_t size,
                                 mode_t createMask) {
  Writer writer(Writer::WriteOnly, Writer::Truncate, *this, fname, createMask);
  writer.Pwrite(buf, size, 0);
  writer.SyncAndClose();
}

void geFilePool::ReadSimpleFile(const std::string &fname,
                                void* buf, size_t size) {
  Reader reader(*this, fname);
  reader.Pread(buf, size, 0);
}

void geFilePool::WriteStringFile(const std::string &fname,
                                 const std::string &buf,
                                 mode_t createMask) {
  WriteSimpleFile(fname, buf.data(), buf.size(), createMask);
}
void geFilePool::ReadStringFile(const std::string &fname, std::string *buf) {
  std::uint64_t size = 0;
  time_t mtime;
  if (!khGetFileInfo(fname, size, mtime)) {
    throw khSimpleException("No such file ") << fname;
  }

  buf->resize(size);
  ReadSimpleFile(fname, &(*buf)[0], buf->size());
}

void geFilePool::WriteSimpleFileWithCRC(const std::string &fname,
                                        EndianWriteBuffer &buf,
                                        mode_t createMask) {
  // grow the buffer to make room for the CRC
  buf << std::uint32_t(0);

  // do the write
  Writer writer(Writer::WriteOnly, Writer::Truncate, *this, fname, createMask);
  writer.PwriteCRC(const_cast<char*>(&buf[0]), buf.size(), 0);
  writer.SyncAndClose();
}

void geFilePool::ReadSimpleFileWithCRC(const std::string &fname,
                                       EndianReadBuffer &buf) {
  // get the filesize
  std::uint64_t size = 0;
  time_t mtime;
  if (!khGetFileInfo(fname, size, mtime)) {
    throw khSimpleException("No such file ") << fname;
  }

  // presize the result buffer
  buf.Seek(0);
  buf.resize(size);

  // do the read
  Reader reader(*this, fname);
  reader.PreadCRC(&buf[0], size, 0);

  // prune the CRC
  buf.resize(size - sizeof(std::uint32_t));
}

LockingFileReference geFilePool::GetFileReference(const std::string &fname,
                                                  int createFlags,
                                                  mode_t createMask) {
  // run when pool.mutex is NOT locked
  khLockGuard lock(mutex);
  while (1) {
    FileReferenceImpl* found;
    if (!filerefs.Find(fname, found)) {
      // nothing found for this fname, just make a new one
      FileReference newRef = khRefGuardFromNew
                             (new FileReferenceImpl(*this, fname,
                                                    createFlags,
                                                    createMask));
      filerefs.Add(fname, newRef.operator->());
      return LockingFileReference(*this, newRef);
    }

    if (found->operationPending) {
      // Another thread is opening or closing this file
      // wait until something changes and try again
      WaitForChanges_locked();
      continue;
    }

    if (FileReferenceImpl::IsWriter(createFlags)) {
      if (found->IsWriter()) {
        throw khSimpleException
          ("Internal Error: Unable to create Writer for ")
            << fname << " another writer already exists";
      } else {
        throw khSimpleException
          ("Internal Error: Unable to create Writer for ")
            << fname << " a reader already exists";
      }
    } else {
      if (found->IsWriter()) {
        throw khSimpleException
          ("Internal Error: Unable to create Reader for ")
            << fname << " a writer already exists";
      }
    }
    return LockingFileReference(*this, khRefGuardFromThis_(found));
  }
}

bool geFilePool::FindOldestUnusedReservation_locked(FileReference *found) {
  for (FileMap::iterator file = filerefs.begin();
       file != filerefs.end(); ++file) {
    if ((*file)->reservation && ((*file)->reservation.refcount() == 1)
         && !((*file)->operationPending)) {
      // this file has a reservation and the only user of the reservation is
      // this FileReference
      *found = khRefGuardFromThis_(*file);
      return true;
    }
  }
  return false;
}

void geFilePool::ReduceFdCount_locked(void) {
  assert(numFdsUsed > 0);
  --numFdsUsed;
}

void geFilePool::IncreaseFdCount_locked(void) {
  assert(numFdsUsed < maxNumFds);
  if (++numFdsUsed > maxFdsUsed) {
    maxFdsUsed = numFdsUsed;
  }
}



// ****************************************************************************
// ***  geFilePool::Reader
// ****************************************************************************
geFilePool::Reader::Reader(geFilePool &pool, const std::string &fname) :
    fileref(pool.GetFileReference(fname, O_RDONLY | O_LARGEFILE, 0))
{
}

geFilePool::Reader::~Reader(void) {
  // nothing to do, fileref's destructor takes care of it for me
}

 std::uint64_t geFilePool::Reader::Filesize(void) const {
  return fileref->Filesize();
}

void geFilePool::Reader::Pread(void *buffer, size_t size, off64_t offset) {
  fileref->Pread(buffer, size, offset);
}

void geFilePool::Reader::Pread(std::string &buffer, size_t size,
                               off64_t offset) {
  buffer.resize(size);
  Pread(&buffer[0], size, offset);
}

void geFilePool::Reader::PreadCRC(void *buffer, size_t size, off64_t offset) {
  Pread(buffer, size, offset);
  size_t data_len = size - sizeof(std::uint32_t);
  std::uint32_t computed_crc = Crc32(buffer, data_len);
  std::uint32_t file_crc;
  FromLittleEndianBuffer(&file_crc,
                         reinterpret_cast<char*>(buffer) + data_len);
  if (computed_crc != file_crc) {
    throw khSimpleException("geFilePool::Reader::PreadCRC: CRC mismatch");
  }
}
void geFilePool::Reader::PreadCRC(std::string &buffer, size_t size,
                                  off64_t offset) {
  buffer.resize(size);
  PreadCRC(&buffer[0], size, offset);
}


// ****************************************************************************
// ***  geFilePool::Writer
// ****************************************************************************
geFilePool::Writer::Writer(WriteStyle wStyle, TruncateStyle tStyle,
                           geFilePool &pool, const std::string &fname_,
                           mode_t createMask) :
    wb_size_(0),
    wb_buffer_offset_(0),
    wb_length_(0),
    fileref(pool.GetFileReference(fname_,
                                  int(wStyle) | int(tStyle) |
                                  O_CREAT | O_LARGEFILE,
                                  createMask))
{
}

geFilePool::Writer::~Writer(void) noexcept(false) {
  if (wb_length_ != 0) {
    throw khSimpleException("geFilePool::~Writer: write buffer not flushed.");
  }
}

void geFilePool::Writer::BufferWrites(size_t write_buffer_size) {
  khLockGuard lock(wb_lock_);
  FlushWriteBufferLocked();
  wb_size_ = write_buffer_size;
  wb_buffer_offset_ = 0;
  wb_length_ = 0;
  if (wb_size_ > 0) {
    write_buffer_ = TransferOwnership(new char[wb_size_]);
  } else {
    write_buffer_.clear();
  }
}

void geFilePool::Writer::Pwrite(const void *buffer, size_t size,
                                off64_t offset) {
  // If the new write will not fit in the write buffer, then just write to disk,
  // ignoring the buffer.  This includes the unbuffered case.
  if (size > wb_size_) {
    fileref->Pwrite(buffer, size, offset);
  } else {
    // Buffer the writes.
    // Protect against multiple threads accessing the write buffer at once.
    khLockGuard lock(wb_lock_);

    // If we can't add the new write to buffer, then
    // flush the buffer.  To add to the buffer, the new write must be:
    //   - appending to the end of the currently-buffered data, and
    //   - must fit in the buffer size.
    if (offset != static_cast<off64_t>(wb_buffer_offset_ + wb_length_) ||
        wb_length_ + size > wb_size_) {
      FlushWriteBufferLocked();
      wb_buffer_offset_ = offset;
      wb_length_ = 0;
    }

    // Now append the new data to the write buffer.
    assert(write_buffer_);
    assert(wb_length_ + size <= wb_size_);
    memcpy(&write_buffer_[0] + wb_length_, buffer, size);
    wb_length_ += size;
  }
}

void geFilePool::Writer::PwriteCRC(void *buffer, size_t size, off64_t offset) {
  size_t data_len = size - sizeof(std::uint32_t);
  ToLittleEndianBuffer(static_cast<char*>(buffer) + data_len,
                       Crc32(buffer, data_len));
  Pwrite(buffer, size, offset);
}

void geFilePool::Writer::SyncAndClose(void) {
  FlushWriteBuffer();
  fileref->Close();
}

void geFilePool::Writer::Pread(void *buffer, size_t size, off64_t offset)
{
  FlushWriteBuffer();
  fileref->Pread(buffer, size, offset);
}

void geFilePool::Writer::Pread(std::string &buffer, size_t size,
                               off64_t offset)
{
  buffer.resize(size);
  Pread(&buffer[0], size, offset);
}

void geFilePool::Writer::PreadCRC(void *buffer, size_t size, off64_t offset)
{
  Pread(buffer, size, offset);
  size_t data_len = size - sizeof(std::uint32_t);
  std::uint32_t computed_crc = Crc32(buffer, data_len);
  std::uint32_t file_crc;
  FromLittleEndianBuffer(&file_crc,
                         reinterpret_cast<char*>(buffer) + data_len);
  if (computed_crc != file_crc) {
    throw khSimpleException("geFilePool::Writer::PreadCRC: CRC mismatch");
  }
}

void geFilePool::Writer::PreadCRC(std::string &buffer, size_t size,
                                  off64_t offset)
{
  buffer.resize(size);
  PreadCRC(&buffer[0], size, offset);
}

void geFilePool::Writer::FlushWriteBuffer() {
  khLockGuard lock(wb_lock_);
  FlushWriteBufferLocked();
}

void geFilePool::Writer::FlushWriteBufferLocked() {
  if (write_buffer_ && wb_length_ != 0) {
    fileref->Pwrite(&write_buffer_[0], wb_length_, wb_buffer_offset_);
  }
  wb_length_ = 0;
}

// ****************************************************************************
// ***  LockingFileReference
// ****************************************************************************
LockingFileReference::LockingFileReference(geFilePool &pool_,
                                           const FileReference &o)
    : FileReference(o), pool(pool_)
{
}

LockingFileReference::~LockingFileReference(void) {
  // run when pool.mutex is NOT locked
  khLockGuard lock(pool.mutex);

  // release my count now (while I hold the lock)
  release();
}
