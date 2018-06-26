// Copyright 2018 Open GEE Contributors
// Unit tests for ServerdbUtilities.

#include <gtest/gtest.h>
#include <string>
#include "common/serverdb/serverdbUtilities.h"


class ServerdbUtilitiesTest : public testing::Test {
protected:
    virtual void SetUp() {}

    virtual void TearDown() {
    }

    std::string GetJavaScriptString() {
        std::string test_string =
        "{"
        "dbType : \"gedb\","
        "isAuthenticated : false,"
        "layers :"
        "["
        "{"
        "description : \"\","
        "id : \"4c8ca722-73cd-11e8-8e36-a969646ae0ac\","
        "isEnabled : true,"
        "isExpandable : false,"
        "isInitiallyOn : true,"
        "isVisible : true,"
        "label : \"Imagery\","
        "lookAt : \"none\""
        "}"
        "]"
        ","
        "serverUrl : \"http://localhost/testdb-v001\""
        "}";
        return test_string;
    }

    std::string GetJSONString() {
        std::string json_string =
        "{"
        "\"dbType\" : \"gedb\","
        "\"isAuthenticated\" : false,"
        "\"layers\" :"
        "["
        "{"
        "\"description\" : \"\","
        "\"id\" : \"4c8ca722-73cd-11e8-8e36-a969646ae0ac\","
        "\"isEnabled\" : true,"
        "\"isExpandable\" : false,"
        "\"isInitiallyOn\" : true,"
        "\"isVisible\" : true,"
        "\"label\" : \"Imagery\","
        "\"lookAt\" : \"none\""
        "}"
        "]"
        ","
        "\"serverUrl\" : \"http://localhost/testdb-v001\""
        "}";
        return json_string;
    }

    ServerdbUtilities utilities_;
};

TEST_F(ServerdbUtilitiesTest, TestParseValidJSON) {
    std::string test_string = this->GetJavaScriptString();
    std::string result = utilities_.ParseValidJSON(test_string);
    std::string expected_string = this->GetJSONString();
    EXPECT_EQ(expected_string, result);
}


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
