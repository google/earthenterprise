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

// Wrapper class that takes an eta dbroot and saves as a proto-buffer dbroot.

#include "common/gedbroot/eta2proto_dbroot.h"

#include "common/gedbroot/eta_dbroot.h"
#include "common/khFileUtils.h"
#include "common/khSimpleException.h"
#include "common/khstl.h"
#include "common/geInstallPaths.h"
#include "common/khConstants.h"

using std::string;

namespace gedbroot {

geEta2ProtoDbroot::geEta2ProtoDbroot(geProtoDbroot *protoDbroot)
  : epoch_(0)
  , converter_(protoDbroot) {
  if (!protoDbroot) {
    notify(NFY_FATAL, "Invalid proto dbroot passed to geEta2ProtoDbroot.");
  }
}

geEta2ProtoDbroot::~geEta2ProtoDbroot() {
  // Define in .cc to reduce linker dependencies.
}

const std::string& geEta2ProtoDbroot::EtaContent() const {
  return etaContent_;
}

bool geEta2ProtoDbroot::ConvertFile(const std::string &filename,
                                    const int database_version) {
  // load content from ETA dbroot
  if (!Load(filename, database_version)) {
    return false;
  }

  // Decoding ETA content to proto.
  return converter_.ParseETADbRoot(etaContent_, epoch_);
}

bool geEta2ProtoDbroot::Convert(const std::string &content,
                                const int database_version) {
  if (ReadBinaryContent(content) ||
      ReadAsciiContent(content, database_version)) {
    // Decoding ETA content to proto.
    return converter_.ParseETADbRoot(etaContent_, epoch_);
  }
  return false;
}

bool geEta2ProtoDbroot::ConvertFromBinary(const std::string &content) {
  if (ReadBinaryContent(content)) {
    // Decoding ETA contents to proto.
    return converter_.ParseETADbRoot(etaContent_, epoch_);
  }
  return false;
}

bool geEta2ProtoDbroot::Load(const std::string &filename,
                             const int database_version) {
  std::string content;
  khReadStringFromFile(filename, content);

  return ReadBinaryContent(content) || ReadAsciiContent(content,
                                                        database_version);
}

bool geEta2ProtoDbroot::ReadBinaryContent(const std::string &content) {
  return EtaDbroot::DecodeBinary(content, &etaContent_, &epoch_);
}

bool geEta2ProtoDbroot::ReadAsciiContent(const std::string &content,
                                         const int database_version) {
  if (!EtaDbroot::IsEtaDbroot(content, EtaDbroot::kExpectAscii) ||
      !khReadStringFromFile(kDbrootTemplatePath, etaContent_)) {
    return false;
  }

  // merge the results
  epoch_ = database_version;
  etaContent_ += content;
  return true;
}

}  // namespace gedbroot
