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

//

#ifndef COMMON_GEINDEX_PACKETFILEADAPTINGTRAVERSER_H__
#define COMMON_GEINDEX_PACKETFILEADAPTINGTRAVERSER_H__

#include "Traverser.h"

class PacketIndexReader;

namespace geindex {


// ****************************************************************************
// ***  Traverse a Packetfile but make it look like we're travesing an
// ***  AllInfo index
// ****************************************************************************
template <class TypedBucket>
class PacketFileAdaptingTraverserBase :
      public AdaptingTraverserBase<TypedBucket> {
 public:
  PacketFileAdaptingTraverserBase(geFilePool &file_pool,
                                  const std::string &merge_source_name,
                                  TypedEntry::TypeEnum type,
                                  std::uint32_t version,
                                  std::uint32_t channel,
                                  const std::string packetfile);
  ~PacketFileAdaptingTraverserBase(void);

  virtual bool Advance();
  virtual void Close();
  inline bool Active() {
    return have_current_;
  }

 private:
  khDeleteGuard<PacketIndexReader> packet_index_reader_;
  std::uint32_t version_;
  std::uint32_t channel_;
  bool have_current_;

  void ReadNext(void);
};

typedef PacketFileAdaptingTraverserBase<UnifiedBucket>
UnifiedPacketFileAdaptingTraverser;

typedef class PacketFileAdaptingTraverserBase<AllInfoBucket>
AllInfoPacketFileAdaptingTraverser;

} // namespace geindex

#endif // COMMON_GEINDEX_PACKETFILEADAPTINGTRAVERSER_H__
