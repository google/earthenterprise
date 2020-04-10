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

#ifndef GOOGLE3_KEYHOLE_TOOLS_DBROOT_ETA_DBROOT_UTILS_H_
#define GOOGLE3_KEYHOLE_TOOLS_DBROOT_ETA_DBROOT_UTILS_H_

#include <string>
#include <vector>

#include "common/gedbroot/proto_dbroot.h"
#include "common/gedbroot/tools/eta_parser.h"

namespace keyhole {
namespace eta_utils {

// Convenience class for reading simple variables from EtaStruct.
class EtaReader {
 public:
  explicit EtaReader(const EtaStruct* eta_struct)
      : eta_struct_(eta_struct) {
  }
  const std::string GetString(const std::string& name,
                              const std::string& default_val) const;
  std::int32_t GetInt(const std::string& name, std::int32_t default_val) const;
  std::uint32_t GetUInt(const std::string& name, std::uint32_t default_val) const;
  float GetFloat(const std::string& name, float default_val) const;
  bool GetBool(const std::string& name, bool default_val) const;

 private:
  const EtaStruct* const eta_struct_;
};

// Translates a list of supported domains into structured data.
void TranslateSupportedDomains(const std::string& mfe_supported_domains,
                               dbroot::EndSnippetProto* end_snippet);

// Helper function that extracts a list of values from a named variable that
// have the given type.
bool GetListFromDocument(const EtaDocument& eta_doc,
                         const char* list_name,
                         const char* type,
                         std::vector<EtaStruct*>* output);

// Standalone functions that parse parts of dbroot in ETA format into
// dbroot v2 protocol format.
void ParseDrawFlags(int flags, dbroot::StyleAttributeProto* style);
void ParseStyleAttributes(const EtaDocument& eta_doc,
                          dbroot::DbRootProto* dbroot);
void ParseStyleMaps(const EtaDocument& eta_doc, dbroot::DbRootProto* dbroot);
void ParseProviders(const EtaDocument& eta_doc, dbroot::DbRootProto* dbroot);

void ParseLookAtString(const std::string& look_at_string,
                       dbroot::NestedFeatureProto* nested_feature);

void ParseEndSnippet(const EtaDocument& doc, dbroot::DbRootProto* dbroot);

void ParseModel(const EtaStruct& root, dbroot::EndSnippetProto* snippet);

void ParseClientOptions(const EtaStruct& root,
                        dbroot::EndSnippetProto* snippet);

void ParseFetchingOptions(const EtaStruct& root,
                          dbroot::EndSnippetProto* snippet);

void ParseTimeMachineOptions(const EtaStruct& root,
                             dbroot::EndSnippetProto* snippet);

void ParseCSIOptions(const EtaStruct& root, dbroot::EndSnippetProto* snippet);

void ParseSearchTabs(const EtaDocument& eta_doc,
                     dbroot::EndSnippetProto* snippet);

void ParseCobrandInfo(const EtaDocument& eta_doc,
                      dbroot::EndSnippetProto* snippet);
void ParseValidDatabases(const EtaDocument& eta_doc,
                         dbroot::EndSnippetProto* snippet);
// This method translates encoded \n characters in ETA into actual line breaks.
// ETA doesn't support strings with line breaks in them, but protocol buffers
// have no problem with that.
void ParseClientConfigScripts(const EtaDocument& eta_doc,
                              dbroot::EndSnippetProto* dbroot);

void ParseSwoopParams(const EtaStruct& root, dbroot::EndSnippetProto* snippet);

bool ParsePostingServer(const EtaStruct& root,
                        const char* server_base_name,
                        dbroot::PostingServerProto* posting_server);

void ParsePlanetaryDatabases(const EtaDocument& eta_doc,
                             dbroot::EndSnippetProto* snippet);

void ParseLogServer(const EtaStruct& root, dbroot::EndSnippetProto* snippet);

}  // namespace eta_utils
}  // namespace keyhole

#endif  // GOOGLE3_KEYHOLE_TOOLS_DBROOT_ETA_DBROOT_UTILS_H_
