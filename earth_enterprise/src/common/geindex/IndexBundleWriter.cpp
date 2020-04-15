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

//

#include "IndexBundleWriter.h"
#include <filebundle/filebundlewriter.h>
#include <khEndian.h>

namespace geindex {

// ****************************************************************************
// ***  BundleFreePool
// ****************************************************************************
BundleFreePool::BundleFreePool(std::uint32_t minBlockSize) :
    kMinBlockSize(minBlockSize),
    totalFreeSpace(0),
    wastedSpace(0)
{
}

void BundleFreePool::Add(const BundleAddr &addr) {
  if (addr.Size() >= kMinBlockSize) {

    // add new BundleAddr to list
    // IMPLEMENT ME;

    totalFreeSpace += addr.Size();
  } else {
    wastedSpace += addr.Size();
  }
}

BundleAddr BundleFreePool::Get(std::uint32_t size) {
  BundleAddr freeAddr;

  // seach free pool for a block to satisfy this request
  // remove it from list
  // IMPLEMENT ME;

  if (freeAddr) {
    totalFreeSpace -= freeAddr.Size();

    if (freeAddr.Size() > size) {
      Add(BundleAddr(freeAddr.Offset()+size, freeAddr.Size() - size));
      freeAddr = BundleAddr(freeAddr.Offset(), size);
    }
  }
  return freeAddr;
}


// ****************************************************************************
// ***  IndexBundleWriter
// ****************************************************************************
void IndexBundleWriter::Close(void) {
  header.wastedSpace +=
    freePool.TotalFreeSpace() + freePool.TotalWastedSpace();
  bundleWriter.Close();

  LittleEndianWriteBuffer buf;
  buf << header;
  filePool.WriteSimpleFileWithCRC(Header::HeaderPath(fname), buf);
}

IndexBundleWriter::IndexBundleWriter(geFilePool &filePool_,
                                     const std::string &fname_,
                                     bool deltaMode,
                                     const std::string &desc,
                                     bool slotsAreSingle,
                                     std::uint32_t num_write_buffers) :
    IndexBundle(TransferOwnership
                (deltaMode ? new FileBundleUpdateWriter(filePool_, fname_) :
                 new FileBundleWriter(filePool_, fname_)),
                desc,
                slotsAreSingle),
    bundleWriter(static_cast<FileBundleWriter&>(*fileBundle)),
    filePool(filePool_),
    fname(fname_)
{
#ifndef SINGLE_THREAD
  delayed_write_queue_ = 0;
  for (unsigned int i = 0; i < num_write_buffers; ++i) {
    buffer_cache_.Push(TransferOwnership(
                           new LittleEndianWriteBuffer(8096)));
  }
#else
  buffer_cache_ = TransferOwnership(new LittleEndianWriteBuffer(8096));
#endif
}

IndexBundleWriter::~IndexBundleWriter(void) {
}


void IndexBundleWriter::WriteAndReturn(const BundleAddr &addr,
                                       CachedBuffer buf) {
  bundleWriter.WriteAt(addr, &(*buf)[0]);
  ReturnWriteBuffer(buf);
}

IndexBundleWriter::CachedBuffer IndexBundleWriter::GetWriteBuffer(void) {
#ifndef SINGLE_THREAD
  BufferCache::PullHandle h;
  buffer_cache_.ReleaseOldAndPull(h);
  assert(h);
  assert(h->Data());
  h->Data()->reset();
  return h->Data();
#else
  buffer_cache_->reset();
  return buffer_cache_;
#endif
}

void IndexBundleWriter::ReturnWriteBuffer(CachedBuffer buf) {
#ifndef SINGLE_THREAD
  buffer_cache_.Push(buf);
#else
  buffer_cache_ = buf;
#endif
}

BundleAddr IndexBundleWriter::GetNewAddr(const BundleAddr &prevAddr,
                                         std::uint32_t newSize) {
  bool prevWritable = prevAddr && 
                      bundleWriter.IsWriteable(prevAddr.Offset());
  if (!newSize) {
    // bucket is empty - somebody must have deleted the last thing
    if (prevWritable) {
      freePool.Add(prevAddr);
    }
    return BundleAddr();
  } else {
    BundleAddr newAddr;
    if ((newSize <= prevAddr.Size()) && prevWritable) {
      // we can reuse our previous disk block
      newAddr = BundleAddr(prevAddr.Offset(), newSize);

      // return any excess to the free pool
      freePool.Add(BundleAddr(prevAddr.Offset() + newSize,
                              prevAddr.Size() - newSize));
    } else {
      // we must get a new disk block
      if (prevWritable) {
        freePool.Add(prevAddr);
      }
      if (!(newAddr = freePool.Get(newSize))) {
        std::uint64_t offset = bundleWriter.AllocateAppend(newSize);
        newAddr = BundleAddr(offset, newSize);
      }
    }
    return newAddr;
  }
}

BundleAddr IndexBundleWriter::StoreAndReturnBuffer(const BundleAddr &prevAddr,
                                                   CachedBuffer buf) {
  BundleAddr newAddr(GetNewAddr(prevAddr, buf->CurrPos()));

  if (newAddr) {
#ifndef SINGLE_THREAD
    if (delayed_write_queue_) {
      delayed_write_queue_->Push(
          khFunctor<void>(std::mem_fun(&IndexBundleWriter::WriteAndReturn),
                          this, newAddr, buf));
    } else {
      WriteAndReturn(newAddr, buf);
    }
#else
  WriteAndReturn(newAddr, buf);
#endif
  }
  return newAddr;
}


} // namespace geindex
