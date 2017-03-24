// Copyright 2017 Google Inc.
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

#include <khSimpleException.h>
#include <khEndian.h>
#include <khFileUtils.h>
#include <filebundle/filebundlereader.h>
#include "IndexBundleReader.h"

namespace geindex {

IndexBundleReader::IndexBundleReader(geFilePool &filePool,
                                     const std::string &header_dir) :
    IndexBundle(filePool,
                TransferOwnership(new FileBundleReader(filePool, header_dir))) {
}

IndexBundleReader::IndexBundleReader(geFilePool &filePool,
                                     const std::string &header_dir,
                                     const std::string &prefix) :
    IndexBundle(filePool, TransferOwnership(
        new FileBundleReader(filePool, header_dir, prefix)), prefix) {
}

void IndexBundleReader::LoadWithCRC(const BundleAddr &addr,
                                    EndianReadBuffer &buf) {
  buf.Seek(0);
  buf.resize(addr.Size());
  GetFileBundleReader().ReadAtCRC(addr.Offset(), &buf[0], addr.Size());
  buf.resize(addr.Size()-FileBundle::kCRCsize);
}

void IndexBundleReader::LoadWithoutCRC(const BundleAddr &addr,
                                       EndianReadBuffer &buf) {
  buf.Seek(0);
  buf.resize(addr.Size());
  GetFileBundleReader().ReadAt(addr.Offset(), &buf[0], addr.Size());
}

} // namespace geindex
