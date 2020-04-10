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


// Test helpers for tests needing ordered or random quadtree paths

#include <vector>
#include <third_party/rsa_md5/crc32.h>
#include <quadtreepath.h>

namespace qtpathgen {

// TestRecord - record with path, position, size, and optional data

class TestRecord {
 public:
  TestRecord(QuadtreePath qt_path, std::uint64_t position)
      : qt_path_(qt_path),
        position_(position) {
  }
  TestRecord() {}
  virtual ~TestRecord() {}
  QuadtreePath qt_path() const { return qt_path_; }
  void set_qt_path(QuadtreePath new_qt_path) { qt_path_ = new_qt_path; }
  std::uint64_t position() const { return position_; }
  void set_position(std::uint64_t new_position) { position_ = new_position; }
  virtual std::vector<char> *mutable_buffer() = 0;   // returns NULL if no buffer
  virtual size_t buffer_size() const = 0;
  virtual void set_buffer_size(size_t new_size) = 0;

  bool operator<(const TestRecord &other) const {
    return qt_path_ < other.qt_path_;
  }
 private:
  QuadtreePath qt_path_;
  std::uint64_t position_;
};

// TestDataRecord - has actual data storage

class TestDataRecord : public TestRecord {
 public:
  TestDataRecord(QuadtreePath qt_path,
                 size_t buffer_size,
                 std::uint64_t position = 0)
      : TestRecord(qt_path, position),
        buffer_(buffer_size) {
  }
  TestDataRecord() {}
  virtual std::vector<char> *mutable_buffer() { return &buffer_; }
  virtual size_t buffer_size() const { return buffer_.size(); }
  virtual void set_buffer_size(size_t new_size) { buffer_.resize(new_size); }
  void *mutable_buffer_ptr() { return &buffer_[0]; }
  const void *buffer_ptr() const { return &buffer_[0]; }

 private:
  std::vector<char> buffer_;
};

// TestPositionRecord - has dummy position and size, no actual data

class TestPositionRecord : public TestRecord {
 public:
  TestPositionRecord(QuadtreePath qt_path,
                 size_t buffer_size,
                 std::uint64_t position = 0)
      : TestRecord(qt_path, position),
        buffer_size_(buffer_size) {
  }
  TestPositionRecord() {}
  virtual std::vector<char> *mutable_buffer() { return NULL; }
  virtual size_t buffer_size() const { return buffer_size_; }
  virtual void set_buffer_size(size_t new_size) { buffer_size_ = new_size; }

 private:
  size_t buffer_size_;
};


QuadtreePath HashPath(std::uint32_t hash_val);

// Generate a vector of records of random quadtree path.  If actual
// vector type is TestDataRecord, generates random data with 0
// positions.  If type is TestPosition Record, fills in random
// positions and sizes.


template <class RecordType>
void BuildRandomRecords(std::vector<RecordType> &records,
                        int record_count,
                        size_t max_size) {
  records.clear();
  records.resize(record_count);
  for (int i = 0; i < record_count; ++i) {
    size_t buffer_size = kCRC32Size + 1 + static_cast<size_t>
                         ((static_cast<float>(random())
                           / RAND_MAX * max_size));
    records[i].set_buffer_size(buffer_size);
    records[i].set_qt_path(HashPath(i));
      
    std::vector<char> *buffer = records[i].mutable_buffer();
    if (buffer != NULL) {
      for (size_t j = 0; j < buffer_size; ++j) {
        (*buffer)[j] = random() & 0xFF;
      }
    } else {
      records[i].set_position(random());
    }
  }
}

} // namespace qtpathgen
