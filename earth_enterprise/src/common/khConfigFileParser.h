/*
 * Copyright 2017 Google Inc. Copyright 2019 the Open GEE Contributors.
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

#ifndef __khConfigFileParser_h
#define __khConfigFileParser_h

#include <string>
#include <vector>
#include <map>
#include <istream>
#include <exception>

class khConfigFileParserException : public std::exception 
{
protected:
    std::string str;
public:
    const char* what() const noexcept { return str.c_str(); }
};
class FileNotPresentException : public khConfigFileParserException
{
public:
    FileNotPresentException(const std::string&);
};
class OptionsEmptyException : public khConfigFileParserException
{
public:
    OptionsEmptyException();
};
class KeyNotPresentException : public khConfigFileParserException
{
public:
    KeyNotPresentException(const std::string&);
};
class ValueNotPresentException : public khConfigFileParserException
{
public:
    ValueNotPresentException(const std::string&);
};
class ValidateIntegersException : public khConfigFileParserException
{
public:
    ValidateIntegersException();
};

class khConfigFileParser
{
private:
    std::vector<std::string> options;
    std::map<std::string,std::string> contents;

    bool sanitize(std::string&);
    void split(const std::string&, std::string&, std::string&);
    bool isKeyPresent(const std::string&);
    void validateIntegerValues();

public:
    
    void addOption(std::string);
    void parse(const std::string&);
    void parse(std::istream&);
    void clearOptions() { options.clear(); }

    int size()   { return contents.size();   }
    const std::string& at(const std::string&);
    const std::string& operator[](const std::string&);
    
    // allow for range looping
    std::map<std::string,std::string>::const_iterator 
    begin() { return contents.begin(); }
    std::map<std::string,std::string>::const_iterator 
    end() { return contents.end(); }	

    // delete everything but constructor/destructor
    khConfigFileParser()                                     = default;
    ~khConfigFileParser()                                    = default;

    khConfigFileParser(const khConfigFileParser&)            = delete;
    khConfigFileParser(khConfigFileParser&&)                 = delete;
    khConfigFileParser& operator=(const khConfigFileParser&) = delete;
    khConfigFileParser& operator=(khConfigFileParser&&)      = delete;
};

#endif 
