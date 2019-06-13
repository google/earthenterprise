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
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "khxml.h"
#include "khdom.h"

using namespace std;
using namespace khxml;

// These tests are focused on testing the Open GEE XML code, not the Xerces
// code. Thus, they do not try to hit every possible error condition; rather
// they hit at least one error condition in every function and make sure Open
// GEE handles it gracefully.
// The functions tested here are declared in khdom.h and defined in khxml.cpp.

const string TEST_DIR = "fusion/testdata/khxml";
const string VALID_XML = TEST_DIR + "/valid.xml";

class KhxmlTest : public ::testing::Test {
  protected:
    void SetUp() override {
      stringstream params;
      params << "INIT_HEAP_SIZE=16384" << endl
             << "MAX_HEAP_SIZE=131072" << endl
             << "BLOCK_SIZE=4096" << endl
             << "PURGE=1" << endl
             << "PURGE_LEVEL=1" << endl;
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

std::string getAttribute(DOMNode * child, const std::string & name) {
  DOMNamedNodeMap * attributes = child->getAttributes();
  if (!attributes) return "no attributes";
  if (attributes->getLength() != 1) return "wrong length";
  DOMNode * attr = attributes->getNamedItem(ToXMLStr(name));
  if (!attr) return "missing attribute";
  const XMLCh * value = attr->getNodeValue();
  if (!value) return "no value";
  return FromXMLStr(value);
}

int getValue(DOMNode * child) {
  DOMNode * grandchild = child->getFirstChild();
  if (!grandchild) return -2;
  const XMLCh * value = grandchild->getNodeValue();
  if (!value) return -1;
  string valStr = FromXMLStr(value);
  return stoi(valStr);
}

template <class String>
void fromStringMapTest() {
  map<String, int> stringMap;
  stringMap["abc"] = 1;
  stringMap["def"] = 10;
  stringMap["ghi"] = 3;
  unique_ptr<GEDocument> doc = CreateEmptyDocument("test");
  DOMElement * elem = doc->getDocumentElement();
  AddElement(elem, "data", stringMap);
  
  DOMNodeList * dataElem = elem->getElementsByTagName(ToXMLStr("data"));
  ASSERT_EQ(dataElem->getLength(), 1) << "Wrong number of data elements extracted from XML";
  ASSERT_EQ(FromXMLStr(dataElem->item(0)->getNodeName()), "data") << "Unexpected tag name when extracting data element";
  DOMNode * child = dataElem->item(0)->getFirstChild();
  ASSERT_EQ(FromXMLStr(child->getNodeName()), "item") << "Invalid parent element when writing map of strings";
  ASSERT_EQ(getAttribute(child, "key"), "abc") << "Invalid key when writing map of strings";
  ASSERT_EQ(getValue(child), 1) << "Invalid value when writing map of strings";
  child = child->getNextSibling();
  ASSERT_EQ(FromXMLStr(child->getNodeName()), "item") << "Invalid parent element when writing map of strings";
  ASSERT_EQ(getAttribute(child, "key"), "def") << "Invalid key when writing map of strings";
  ASSERT_EQ(getValue(child), 10) << "Invalid value when writing map of strings";
  child = child->getNextSibling();
  ASSERT_EQ(FromXMLStr(child->getNodeName()), "item") << "Invalid parent element when writing map of strings";
  ASSERT_EQ(getAttribute(child, "key"), "ghi") << "Invalid key when writing map of strings";
  ASSERT_EQ(getValue(child), 3) << "Invalid value when writing map of strings";
  child = child->getNextSibling();
  ASSERT_EQ(child, nullptr) << "Unexpected element";
}

TEST_F(KhxmlTest, StdStringMapToElement) {
  fromStringMapTest<string>();
}

TEST_F(KhxmlTest, SharedStringMapToElement) {
  fromStringMapTest<SharedString>();
}

TEST_F(KhxmlTest, QStringMapToElement) {
  fromStringMapTest<QString>();
}

template <class String>
void toStringTest() {
  string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "<test>expected_value</test>";
  unique_ptr<GEDocument> doc = ReadDocumentFromString(xml, "dummy");
  DOMElement * elem = doc->getDocumentElement();
  ASSERT_EQ(FromXMLStr(elem->getTagName()), "test") << "Invalid tag name when reading string from XML";
  String value;
  FromElement(elem, value);
  ASSERT_EQ(value, "expected_value") << "Could not read string value from XML correctly";
}

TEST_F(KhxmlTest, StdStringReadString) {
  toStringTest<string>();
}

TEST_F(KhxmlTest, SharedStringReadString) {
  toStringTest<SharedString>();
}

TEST_F(KhxmlTest, QStringReadString) {
  toStringTest<QString>();
}

template <class String>
void runXmlToStringMapTest(const std::string & xml) {
  unique_ptr<GEDocument> doc = ReadDocumentFromString(xml, "dummy");
  DOMElement * elem = doc->getDocumentElement();
  ASSERT_EQ(FromXMLStr(elem->getTagName()), "data") << "Invalid tag name when reading string from XML";
  map<String, int> results;
  FromElement(elem, results);
  ASSERT_EQ(results["xyz"], 42) << "Error reading map of strings from XML";
  ASSERT_EQ(results["wvu"], 12) << "Error reading map of strings from XML";
}

template <class String>
void toStringMapTest() {
  // There are two methods of parsing maps from XML. We test both.
  string newxml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "<data>\n"
         "<item key=\"xyz\">42</item>\n"
         "<item key=\"wvu\">12</item>\n"
      "</data>\n";
  runXmlToStringMapTest<String>(newxml);
  string oldxml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "<data>\n"
        "<xyz>42</xyz>\n"
        "<wvu>12</wvu>\n"
      "</data>\n";
  runXmlToStringMapTest<String>(oldxml);
}

TEST_F(KhxmlTest, StdStringReadMap) {
  toStringMapTest<string>();
}

TEST_F(KhxmlTest, SharedStringReadMap) {
  toStringMapTest<SharedString>();
}

TEST_F(KhxmlTest, QStringReadMap) {
  toStringMapTest<QString>();
}

template <class String>
void readEmptyStringFromXml() {
  string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "<value></value>\n";
  unique_ptr<GEDocument> doc = ReadDocumentFromString(xml, "dummy");
  DOMElement * elem = doc->getDocumentElement();
  ASSERT_EQ(FromXMLStr(elem->getTagName()), "value") << "Invalid tag name when reading empty string from XML";
  String val;
  FromElement(elem, val);
  ASSERT_EQ(val, String("")) << "Error reading empty string from XML";
}

TEST_F(KhxmlTest, StdStringReadEmpty) {
  readEmptyStringFromXml<string>();
}

TEST_F(KhxmlTest, SharedStringReadEmpty) {
  readEmptyStringFromXml<SharedString>();
}

TEST_F(KhxmlTest, QStringReadEmpty) {
  readEmptyStringFromXml<QString>();
}

template <class String>
void readNonEmptyStringFromXml() {
  string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "<value>qwertyuiop</value>\n";
  unique_ptr<GEDocument> doc = ReadDocumentFromString(xml, "dummy");
  DOMElement * elem = doc->getDocumentElement();
  ASSERT_EQ(FromXMLStr(elem->getTagName()), "value") << "Invalid tag name when reading empty string from XML";
  String val;
  FromElement(elem, val);
  ASSERT_EQ(val, String("qwertyuiop")) << "Error reading empty string from XML";
}

TEST_F(KhxmlTest, StdStringReadNonEmpty) {
  readNonEmptyStringFromXml<string>();
}

TEST_F(KhxmlTest, SharedStringReadNonEmpty) {
  readNonEmptyStringFromXml<SharedString>();
}

TEST_F(KhxmlTest, QStringReadNonEmpty) {
  readNonEmptyStringFromXml<QString>();
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
