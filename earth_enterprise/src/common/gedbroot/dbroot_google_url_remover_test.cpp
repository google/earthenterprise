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


#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>  // NOLINT(readability/streams)
// TODO: get rid of streams
#include <gtest/gtest.h>

#include "common/gedbroot/dbroot_google_url_remover.h"
#include "common/gedbroot/dbroot_google_url_remover_test_data.h"
#include "common/gedbroot/proto_dbroot.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "google/protobuf/text_format.h"

using std::string;
using std::vector;
using std::cerr;
using std::endl;
using std::flush;
using google::protobuf::TextFormat;
using google::protobuf::Message;
using google::protobuf::Descriptor;
using google::protobuf::Reflection;
using google::protobuf::FieldDescriptor;
using keyhole::dbroot::EndSnippetProto;

// Fields that are affected:
// end_snippet.disable_authentication
// end_snippet.client_options.js_bridge_request_whitelist
// end_snippet.autopia_options.metadata_server_url
// end_snippet.autopia_options.depthmap_server_url
// end_snippet.search_info.default_url
// end_snippet.elevation_service_base_url

const int NUM_GOOGLE_DEFAULTED_FIELDS = 5;

// Outside of TEST* macros, gtest's ASSERT_XX macros with some
// relation (_EQ(a, b), _TRUE(a <= b)) give warnings. So in those
// cases we use this.
// Inside a TEST* macro, we just use ASSERT_*
#if !defined(NDEBUG)
#  define ASSERT_OR_HIDE_VAR(expr, var) assert(expr);
#else
#  define ASSERT_OR_HIDE_VAR(expr, var) (void)var;
#endif  // NDEBUG


const char* AUTHENTICATION_REQUIRED =
    "end_snippet {\n"
    "  disable_authentication: false\n"
    "}\n";


const char* CANON_AUTHENTICATION_NOT_REQUIRED =
  "end_snippet {\n"
  // the point of this test
  "  disable_authentication: true\n"
  "  client_options {\n"
  "    js_bridge_request_whitelist: \"\"\n"
  "  }\n"
  "  autopia_options {\n"
  "    metadata_server_url: \"\"\n"
  "    depthmap_server_url: \"\"\n"
  "  }\n"
  "  search_info {\n"
  "    default_url: \"\"\n"
  "  }\n"
  "  elevation_service_base_url: \"\"\n"
  "}\n";

static
bool StringsAreSame(const string& a_dbroot,
                    const string& b_dbroot,
                    bool want_diff_on_failure = true);

// Get "foo" from "foo!" or "foo[]"
static
string CleanedPathElement(const string& path_element) {
  string cleaned = path_element;
  string::size_type pos_lbrack = cleaned.rfind("[");
  if (pos_lbrack != string::npos) {
    cleaned = cleaned.substr(0, pos_lbrack);
  }
  string::size_type pos_bang = cleaned.rfind("!");
  if (pos_bang != string::npos) {
    cleaned = cleaned.substr(0, pos_bang);
  }
  return cleaned;
}

static
bool ReadableFieldPath(
    const Message* msg,
    const string& path_somewhere_below_msg,
    const Message** out_field_owner,
    const Reflection** out_refl_field_owner,
    const FieldDescriptor** out_field_descriptor) {
  msg->CheckInitialized();
  const Descriptor* desc = msg->GetDescriptor();
  const Reflection* refl = msg->GetReflection();
  const FieldDescriptor* msg_fdesc = NULL;
  vector<string> path_elements;
  TokenizeString(path_somewhere_below_msg, path_elements, ".");
  // Stop, one above the last field.
  for (int i = 0, end = path_elements.size(); i < end; ++i) {
    string path_elt = CleanedPathElement(path_elements[i]);
    msg_fdesc = desc->FindFieldByName(path_elements[i]);
    if (i < end - 1) {
      if (msg_fdesc->is_repeated()) {
        if (!refl->FieldSize(*msg, msg_fdesc)) {
          return false;
        } else {
          // If it's repeated, always just choose 0th.
          msg = &(refl->GetRepeatedMessage(*msg, msg_fdesc, 0));
        }
      } else {
        if (!refl->HasField(*msg, msg_fdesc)) {
          return false;
        } else {
          msg = &(refl->GetMessage(*msg, msg_fdesc));
        }
      }
      desc = msg->GetDescriptor();
      refl = msg->GetReflection();
    }
  }

  *out_field_owner = msg;
  *out_refl_field_owner = refl;
  *out_field_descriptor = msg_fdesc;
  return true;
}

static
bool WritableFieldPath(
    Message* msg,
    const string& path_somewhere_below_msg,
    Message** out_field_owner,
    const Reflection** out_refl_field_owner,
    const FieldDescriptor** out_field_descriptor) {
  const Descriptor* desc = msg->GetDescriptor();
  const Reflection* refl = msg->GetReflection();
  const FieldDescriptor* msg_fdesc = NULL;
  vector<string> path_elements;
  TokenizeString(path_somewhere_below_msg, path_elements, ".");
  // Stop, one above the last field.
  for (int i = 0, end = path_elements.size(); i < end; ++i) {
    string path_elt = CleanedPathElement(path_elements[i]);
    msg_fdesc = desc->FindFieldByName(path_elt);
    if (i < end - 1) {
      if (msg_fdesc->is_repeated()) {
        // hardwire everything to the 0th element. We do this because
        // sometimes we write two sibling fields beneath the same
        // repeated message, and we need them to land in the same spot.
        // (Eg one of the urls beneath nested_feature[], requiring channel_id.)
        if (0 < refl->FieldSize(*msg, msg_fdesc)) {
          msg = refl->MutableRepeatedMessage(msg, msg_fdesc, 0);
        } else {
          msg = refl->AddMessage(msg, msg_fdesc);
        }
      } else {
        msg = refl->MutableMessage(msg, msg_fdesc);
      }
      desc = msg->GetDescriptor();
      refl = msg->GetReflection();
    }
  }

  // Leave the rest to the caller. (TODO: maybe use
  // existing infrastructure - FieldAccessor et al).
  *out_field_owner = msg;
  *out_refl_field_owner = refl;
  *out_field_descriptor = msg_fdesc;
  return true;
}

static
// If a part of the path is repeated, we just get the 0th elt.
// Expects the path to be there.
string StringAtFieldPath(
    const Message* base_msg,
    const string& path_somewhere_below_msg) {
  const Message* field_owner = NULL;
  const Reflection* refl_field_owner = NULL;
  const FieldDescriptor* field_descriptor = NULL;

  bool found = ReadableFieldPath(base_msg,
                                 path_somewhere_below_msg,
                                 &field_owner,
                                 &refl_field_owner,
                                 &field_descriptor);
  ASSERT_OR_HIDE_VAR(found, found);

  if (field_descriptor->is_repeated()) {
    // only one, again; dubious.
    return refl_field_owner->GetRepeatedString(
        *field_owner, field_descriptor, 0);
  } else {
    return refl_field_owner->GetString(*field_owner, field_descriptor);
  }
}


static
// If a part of the path is repeated, we just get the 0th elt.
// Expects the path to be there.
bool HasFieldPath(
    const Message* base_msg,
    const string& path_somewhere_below_msg) {
  const Message* field_owner = NULL;
  const Reflection* refl_field_owner = NULL;
  const FieldDescriptor* field_descriptor = NULL;

  bool found = ReadableFieldPath(base_msg,
                                 path_somewhere_below_msg,
                                 &field_owner,
                                 &refl_field_owner,
                                 &field_descriptor);
  return found;
}

static
void CreateGooglishUrlAtPath(
    Message* msg,
    const string& path_somewhere_below_msg,
    const string& googlish_url = "www.google.com") {
  Message* field_owner = NULL;
  const Reflection* refl_field_owner = NULL;
  const FieldDescriptor* field_descriptor = NULL;
  bool found_path =
      WritableFieldPath(msg, path_somewhere_below_msg,
                        &field_owner,
                        &refl_field_owner,
                        &field_descriptor);

  ASSERT_TRUE(found_path);

  if (field_descriptor->is_repeated()) {
    // only one, again; dubious.
    refl_field_owner->AddString(field_owner, field_descriptor, googlish_url);
  } else {
    refl_field_owner->SetString(field_owner, field_descriptor, googlish_url);
  }
  // don't check; sometimes it's known incomplete
  //  msg->CheckInitialized();
}

static
// Calls out to diff, and prints that diff if there is any.
// TODO: wants to be a utility somewhere
bool FilesAreSame(const string& a_fname,
                  const string& b_fname,
                  bool want_diff_on_failure = true) {
  const int CMD_LEN = 500;
  char buf_commandline[CMD_LEN + 1];
  int nwritten = snprintf(buf_commandline, CMD_LEN, "diff %s %s",
                          a_fname.c_str(), b_fname.c_str());

  ASSERT_OR_HIDE_VAR(nwritten <= CMD_LEN, nwritten);

  FILE* diff_results_file = ::popen(buf_commandline, "r");

  string diff_contents;
  char* line = NULL;
  size_t len_line = 0;
  ssize_t nread;
  // getline is a GNU extension
  while ((nread = ::getline(&line, &len_line, diff_results_file)) != -1) {
    diff_contents.append(line);
  }
  if (line)
    ::free(line);

  size_t size_of_diff = diff_contents.length();

  if (ferror(diff_results_file)) {
    cerr << "error reading diff pipe... " << endl;
  }

  if (want_diff_on_failure && 0 < size_of_diff) {
    cerr << "Diff: \n" << diff_contents << endl;
  }

  int rval = pclose(diff_results_file);
  if (rval < 0) {
    cerr << "pclose error: " << strerror(errno) << endl;
  }
  return size_of_diff == 0;
}

// TODO: wants to be a utility somewhere
class ScopedTempFile {
 public:
  // When ScopedTempFile is born, you get just an empty, closed file;
  // all you really get is the filename. However it does remove the
  // file itself upon exit, so it mixes levels, between the name and
  // the content.
  explicit ScopedTempFile(const string& basename)
      : leave_in_place_(false),
        src_(khTmpFilename(basename))
  {}

  ~ScopedTempFile() {
    if (leave_in_place_) {
      cerr << "leaving temp file: " << filename() << endl;
    } else {
      ::remove(src_.c_str());
    }
  }

  // for debugging
  void LeaveInPlace() { leave_in_place_ = true; }

  string const& filename() const { return src_; }

 private:
  bool leave_in_place_;
  string src_;
};

static
void RunTextBasedTestCase(const char* before, const char* after) {
  geProtoDbroot dbroot;
  bool parsed_ok = TextFormat::ParseFromString(before, &dbroot);
  ASSERT_TRUE(parsed_ok);

  RemoveUnwantedGoogleUrls(&dbroot);
  string cleaned_dbroot_text;
  TextFormat::PrintToString(dbroot, &cleaned_dbroot_text);

  bool are_same = StringsAreSame(after, cleaned_dbroot_text);
  ASSERT_TRUE(are_same);
}

// If a field is empty but has a default, if you explicitly request
// the value of that field you'll get the default.
TEST(GoogleUrlRemoverTest, DemonstrateThatEmptyDefaultsNeedMasking) {
  // Should hardwire; would save space.
  geProtoDbroot dbroot;
  const Descriptor* desc_root = dbroot.GetDescriptor();
  const Reflection* refl_root = dbroot.GetReflection();
  const FieldDescriptor* fdesc_root = desc_root->FindFieldByName("end_snippet");

  Message* snippet = refl_root->MutableMessage(&dbroot, fdesc_root);

  const Descriptor* desc_snip = snippet->GetDescriptor();
  const Reflection* refl_snip = snippet->GetReflection();
  const FieldDescriptor* fdesc_snip = desc_snip->FindFieldByName(
    "elevation_service_base_url");
  // No actual data there
  ASSERT_FALSE(refl_snip->HasField(*snippet, fdesc_snip));
  string googlish_default = refl_snip->GetString(*snippet, fdesc_snip);
  ASSERT_EQ(googlish_default, "http://maps.google.com/maps/api/elevation/");
}


TEST(GoogleUrlRemoverTest, DisableAuthenticationRequirement) {
  RunTextBasedTestCase(AUTHENTICATION_REQUIRED,
                       CANON_AUTHENTICATION_NOT_REQUIRED);
}

static
bool UrlCleaned(const char* test_fieldpath, const string& url) {
  geProtoDbroot dbroot;
  // Randomly-chosen field with a default Google url.
  CreateGooglishUrlAtPath(&dbroot, test_fieldpath, url);

  string written_url = StringAtFieldPath(&dbroot, test_fieldpath);
  ASSERT_OR_HIDE_VAR(written_url == url, url)

  RemoveUnwantedGoogleUrls(&dbroot);

  return !HasFieldPath(&dbroot, test_fieldpath);
}

static
// Any user/Google-written, non-default value is ok.
bool DataOverDefaultRemains(const string& url,
                            const char* fieldpath =
                            "end_snippet.search_info.default_url") {
  return !UrlCleaned(fieldpath, url);
}

static
// The main point, that an empty field with a Google url default gets
// an empty string written to it.
bool EmptyButDefaultedUrlCleaned(const char* test_fieldpath) {
  geProtoDbroot dbroot;
  RemoveUnwantedGoogleUrls(&dbroot);

  string hopefully_empty = StringAtFieldPath(&dbroot, test_fieldpath);
  return hopefully_empty.empty();
}

static
// If we write url to a field without a default and call clean,
// is url cleared out? (should be not.)
bool NonDefaultUrlCleaned(const string& url) {
  return UrlCleaned("end_snippet.sky_database_url.value", url);
}

// A harmless url written over top of a field with a Google default is fine.
TEST(GoogleUrlRemoverTest,
     GoogleDefaultedFieldsWithHarmlessValuesAreUntouched) {
  geProtoDbroot dbroot;
  EndSnippetProto* snippet = dbroot.mutable_end_snippet();
  string harmless("www.nasa.gov");
  snippet->set_elevation_service_base_url(harmless);

  RemoveUnwantedGoogleUrls(&dbroot);

  ASSERT_EQ(harmless, snippet->elevation_service_base_url());
}

// A Google url written over top of a field with a Google default is fine.
// (Same test as <GoogleDefaultedFieldsWithHarmlessValuesAreUntouched>
// but stronger.)
TEST(GoogleUrlRemoverTest, LeaveDeliberatelyPlacedGooglelikeDefaultedDomains) {
  ASSERT_TRUE(DataOverDefaultRemains("http://google.com/deep/path"));
  ASSERT_TRUE(DataOverDefaultRemains("http://khmdb.google.com?db=sky"));
  ASSERT_TRUE(DataOverDefaultRemains("www.google.com"));
  ASSERT_TRUE(DataOverDefaultRemains("auth.keyhole.com"));
  ASSERT_TRUE(DataOverDefaultRemains("https://auth.keyhole.com"));
  ASSERT_TRUE(DataOverDefaultRemains("something.google.co.uk/whyme?"));
}

// Google urls in ordinary fields are fine.
TEST(GoogleUrlRemoverTest, NonDefaultedFieldsUntouched) {
  ASSERT_FALSE(NonDefaultUrlCleaned("http://google.com/deep/path"));
  ASSERT_FALSE(NonDefaultUrlCleaned("http://khmdb.google.com?db=sky"));
  ASSERT_FALSE(NonDefaultUrlCleaned("www.google.com"));
  ASSERT_FALSE(NonDefaultUrlCleaned("auth.keyhole.com"));
  ASSERT_FALSE(NonDefaultUrlCleaned("https://auth.keyhole.com"));
  ASSERT_FALSE(NonDefaultUrlCleaned("something.google.co.uk/whyme?"));
}

// The main test, that empty fields with Google urls are 'plugged'
// with empty strings, hiding the Google url.
TEST(GoogleUrlRemoverTest, GoogleDefaultedFieldsAreMasked) {
  string url = "http://www.google.com";
  int i = 0;
  ++i;
  ASSERT_TRUE(EmptyButDefaultedUrlCleaned(
      "end_snippet.client_options.js_bridge_request_whitelist"));
  ++i;
  ASSERT_TRUE(EmptyButDefaultedUrlCleaned(
      "end_snippet.autopia_options.metadata_server_url"));
  ++i;
  ASSERT_TRUE(EmptyButDefaultedUrlCleaned(
      "end_snippet.autopia_options.depthmap_server_url"));
  ++i;
  ASSERT_TRUE(EmptyButDefaultedUrlCleaned(
      "end_snippet.search_info.default_url"));
  ++i;
  ASSERT_TRUE(EmptyButDefaultedUrlCleaned(
      "end_snippet.elevation_service_base_url"));

  ASSERT_EQ(i, NUM_GOOGLE_DEFAULTED_FIELDS);
}

// That we don't blank out /all/ fields with defaults.
TEST(GoogleUrlRemoverTest, NotAllDefaultFieldsCleared) {
  ASSERT_TRUE(DataOverDefaultRemains("http://www.google.com",
                                     "end_snippet.mfe_lang_param"));
}

static
// Not simple string comparison; lets you see any diff. during debugging.
bool StringsAreSame(const string& text_canon,
                    const string& text_actual,
                    bool want_diff_on_failure /*= true*/) {
  ScopedTempFile fname_canon("canon");
  ScopedTempFile fname_actual("actual");

  FILE* fcanon = ::fopen(fname_canon.filename().c_str(), "w");
//  ASSERT_TRUE(fcanon != NULL);
  FILE* factual = ::fopen(fname_actual.filename().c_str(), "w");
//  ASSERT_TRUE(factual != NULL);

  size_t ncanon = ::fwrite(text_canon.c_str(), 1, text_canon.length(),
                           fcanon);
  // GTest's ASSERT_EQ/TRUE give warnings!
  assert(ncanon == text_canon.length());
  (void)ncanon;
  size_t nactual = ::fwrite(text_actual.c_str(), 1, text_actual.length(),
                            factual);
  assert(nactual == text_actual.length());
  (void)nactual;

  ::fclose(fcanon);
  ::fclose(factual);

  return FilesAreSame(fname_canon.filename(), fname_actual.filename(),
                      want_diff_on_failure);
}


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
