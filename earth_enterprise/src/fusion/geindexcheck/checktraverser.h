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


#ifndef FUSION_GEINDEXCHECK_CHECKTRAVERSER_H__
#define FUSION_GEINDEXCHECK_CHECKTRAVERSER_H__

#include <geFilePool.h>
#include <quadtreepath.h>
#include <geindex/IndexBundle.h>
#include <geindex/Traverser.h>
#include <geindex/Entries.h>
#include <packetfile/packetfilereader.h>

namespace geindexcheck {

// CheckTraverser - index traverser to check index.
//
// Define an abstract base traverser class. Templated subclasses will
// be defined for traversing indexes of each entry bucket type.

class CheckTraverser {
 public:
  virtual ~CheckTraverser() {}
  // Return quadtree path of current traversal location
  virtual const QuadtreePath CurrentPath() const = 0;
  // Return number of entries in current traversal location
  virtual size_t CurrentEntryCount() const = 0;
  // Advance to next location in traversal
  virtual bool Advance() = 0;
  // Returns true if traversal finished
  virtual bool Finished() = 0;
  // Returns true if traversal not finished
  virtual bool Active() = 0;
  // Read and check the entries referenced by the current index
  // traversal location
  virtual void ReadCurrentEntryData() = 0;
  // Close the index and any open packet files
  virtual void Close() = 0;
  // Open all the packet files referenced by the index
  void OpenPacketFiles(geFilePool &file_pool);
 protected:
  CheckTraverser() {};
  // Return reference to the index bundle
  virtual const geindex::IndexBundle &GetIndexBundle() const = 0;
  // Vector of readers for open packet files
  khDeletingVector<PacketFileReaderBase> packet_readers;
 private:
};

// Define templated subclass to traverse index based on bucket type of
// index. Uses geindex::Traverser to accomplish the actual traversal.

template <class BucketType>
class SpecificCheckTraverser : public CheckTraverser {
 public:
  SpecificCheckTraverser(geFilePool &file_pool,
                         const std::string &index_path)
      : index_traverser_("SpecificCheckTraverser",
                         file_pool,
                         index_path) {
  }
  virtual const QuadtreePath CurrentPath() const {
    return index_traverser_.Current().qt_path();
  }
  virtual size_t CurrentEntryCount() const {
    return index_traverser_.Current().size();
  }
  virtual bool Advance() {
    return index_traverser_.Advance();
  }
  virtual bool Finished() {
    return index_traverser_.Finished();
  }
  virtual bool Active() {
    return index_traverser_.Active();
  }
  virtual void Close() {
    index_traverser_.Close();
  }

  // Read the data for all the entries in the current bucket
  virtual void ReadCurrentEntryData();
 protected:
  virtual const geindex::IndexBundle &GetIndexBundle() const {
    return index_traverser_.GetIndexBundleReader();
  }
 private:
  geindex::TypedEntry::TypeEnum type_;
  geindex::Traverser<BucketType> index_traverser_;
  DISALLOW_COPY_AND_ASSIGN(SpecificCheckTraverser<BucketType>);
};


} // namespace geindexcheck

#endif  // FUSION_GEINDEXCHECK_CHECKTRAVERSER_H__
