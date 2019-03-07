// Copyright 2019 The Open GEE Contributors
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

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "khxml.h"
#include "khdom.h"

using namespace std;
using namespace khxml;

// These tests are focused on testing the Open GEE XMl code, not the Xerces
// code. Thus, they do not try to hit every possible error condition; rather
// they hit at least one error condition in every function and make sure Open
// GEE handles it gracefully.

const string TEST_DIR = "fusion/testdata/khxml";
const string VALID_XML = TEST_DIR + "/valid.xml";

class KhxmlTest : public ::testing::Test {
  protected:
    void SetUp() override {
      stringstream params;
      params << "INIT_HEAP_SIZE=16384\n"
                "MAX_HEAP_SIZE=131072\n"
                "BLOCK_SIZE=4096\n"
                "PURGE=1\n"
                "PURGE_LEVEL=1\n"
                "DEALLOCATE_ALL=1\n";
      GEXMLObject::initializeXMLParametersFromStream(params);
    }
};

void populateDocument(GEDocument * doc) {
  XMLCh value[] = {'t', 'e', 's', 't', '\0'};
  DOMElement * elem = doc->getDocumentElement();
  elem->appendChild(elem->getOwnerDocument()->createTextNode(value));
}

TEST_F(KhxmlTest, CreateAndWriteDocument)
{
  unique_ptr<GEDocument> doc = CreateEmptyDocument("rootTagname");
  ASSERT_NE(doc, nullptr) << "Failed to create empty document";
  
  // Do some basic operations and make sure they succeed
  populateDocument(doc.get());
  char * filename = tmpnam(nullptr);
  bool status = WriteDocument(doc.get(), filename);
  ASSERT_TRUE(status) << "Failed to write document";
}

TEST_F(KhxmlTest, CreateEmptyDocument_InvalidCharacter)
{
  unique_ptr<GEDocument> doc = CreateEmptyDocument("<<<<");
  ASSERT_EQ(doc, nullptr) << "Document creation with an invalid character should have failed";
}

TEST_F(KhxmlTest, CreateDocumentNoPerms) {
  unique_ptr<GEDocument> doc = CreateEmptyDocument("rootTagname");
  ASSERT_NE(doc, nullptr) << "Failed to create empty document";
  populateDocument(doc.get());
  
  // Try to write a document in a directory with insufficient permissions
  char dirTemplate[] = "/tmp/xmltestdirXXXXXX";
  string dir = mkdtemp(dirTemplate);
  string filename = dir + "/testfile";
  chmod(dir.c_str(), S_IROTH);

  bool status = WriteDocument(doc.get(), filename.c_str());
  ASSERT_FALSE(status) << "Should not have created document because of restrictive permissions";
}

TEST_F(KhxmlTest, ReadDocument) {
  unique_ptr<GEDocument> doc = ReadDocument(VALID_XML);
  ASSERT_NE(doc, nullptr) << "Error reading valid XML file";
}

TEST_F(KhxmlTest, ReadNonExistantDocument) {
  unique_ptr<GEDocument> doc = ReadDocument("notafile.xml");
  ASSERT_EQ(doc, nullptr) << "Reading a non-existant XML file should failed";
}

TEST_F(KhxmlTest, ReadFromString) {
  string docString =
      "<testfile>\n"
      "  <elem1>some data</elem1>\n"
      "  <elem2 attr=\"attribute\">other data</elem2>\n"
      "</testfile>";
  unique_ptr<GEDocument> doc = ReadDocumentFromString(docString, "fake");
  ASSERT_NE(doc, nullptr) << "Could not read document from string";
}

// This test is disabled because I cannot find a way to get the parser to fail
// and throw an exception.
TEST_F(KhxmlTest, DISABLED_ReadInvalidDocFromString) {
  string docString = 
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "(*@&#^*&@#";
  unique_ptr<GEDocument> doc = ReadDocumentFromString(docString, "fake");
  ASSERT_EQ(doc, nullptr) << "Reading an invalid document from a string should have failed";
}

unique_ptr<GEDocument> getInvalidDocument() {
  return unique_ptr<GEDocument>(new GEParsedDocument("notafile.xml"));
}

TEST_F(KhxmlTest, GetElementFromInvalidDocument) {
  // Make sure that trying to read from an invalid document is handled gracefully
  unique_ptr<GEDocument> doc = getInvalidDocument();
  DOMElement * elem = doc->getDocumentElement();
  ASSERT_EQ(elem, nullptr) << "An invalid document should not return an element";
}

TEST_F(KhxmlTest, WriteInvalidToString) {
  unique_ptr<GEDocument> doc = getInvalidDocument();
  string buf;
  bool status = WriteDocumentToString(doc.get(), buf);
  ASSERT_FALSE(status) << "Writing an invalid document to a string should fail";
}

TEST_F(KhxmlTest, WriteInvalidDocument) {
  unique_ptr<GEDocument> doc = getInvalidDocument();
  bool status = WriteDocument(doc.get(), TEST_DIR + "writeinvaliddocument.xml");
  ASSERT_FALSE(status) << "Writing an invalid document to a file should fail";
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
