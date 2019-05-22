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

#include "khConfigFileParser.h"
#include <algorithm>
#include <fstream>
#include "common/khThread.h"
#include <cctype>

FileNotPresentException::FileNotPresentException(const std::string& fn)
{
    str = fn + " is not present";
}

OptionsEmptyException::OptionsEmptyException()
{
    str = "no list of options present";
}

ValidateIntegersException::ValidateIntegersException()
{
    str = "contains non-integer values";
}

ValueNotPresentException::ValueNotPresentException(const std::string& key)
{
    str = key + " has an unset value";
}

KeyNotPresentException::KeyNotPresentException(const std::string& key)
{
    str = key + " is not a valid key";
}

void khConfigFileParser::addOption(std::string option) 
{
    // if empty, return
    if (option.size() == 0) return;
    std::transform(option.begin(), option.end(),
                   option.begin(), ::toupper);
    options.push_back(option);
}

void khConfigFileParser::validateIntegerValues()
{
    for (const auto& a : contents)
    {
        try
        {
            auto num = std::stol(a.second);

            // stol throws exception below whenever the string begins with non-numerical characters
            // but will convert otherwise. we want to disallow this
            auto backToString = std::to_string(num);
            if (a.second != backToString)
                throw ValidateIntegersException();
        }
        catch (const std::invalid_argument& e)
        {
            throw ValidateIntegersException();
        }
    }
}

bool khConfigFileParser::sanitize(std::string& line)
{
    // remove whitespace
 
    line.erase(std::remove_if(line.begin(), line.end(),
        [](char c)
        {
            return isspace(static_cast<unsigned char>(c));
        }), line.end());

    // remove comments
    auto idx = line.find('#');
    if (idx != std::string::npos) 
    {
        line.erase(line.begin()+idx,line.end());
    }
    
    // remove empty lines
    if (!line.size()) return false;
    
    
    // make uppercase
    std::transform(line.begin(), line.end(),
                   line.begin(), ::toupper);
    return true;
}


void khConfigFileParser::split(const std::string& line, std::string& key, std::string& value)
{
    std::stringstream ss(line);
    std::string buf;
    std::vector<std::string> splitLine;
    while(getline(ss, buf, '='))
        splitLine.push_back(buf);
    key = splitLine[0];
    if (splitLine.size() == 2)
    {
        value = splitLine[1];
    }
}

bool khConfigFileParser::isKeyPresent(const std::string& key)
{
    for (const auto& a : options)
    {
        if (a == key) return true;
    }
    return false;
}

static khMutexBase xmlParmsLock = KH_MUTEX_BASE_INITIALIZER;

void khConfigFileParser::parse(std::istream& fileContents)
{
    std::string line;
    contents.clear(); //clear out the old contents
    while (getline(fileContents,line))
    {
        if (sanitize(line))
        {
            std::string key, value;
            split(line,key,value);
            // Keys not specified as options will be ignored
            if (isKeyPresent(key))
            {
                if (value.size() == 0)
                {
                    throw ValueNotPresentException(key);
                }
                contents.insert(std::pair<std::string,std::string>(key,value));
            }
            else
            {
                notify(NFY_WARN, "Invalid key in configuration file: %s", key.c_str());
            }
        }
    }
    validateIntegerValues();
}

void khConfigFileParser::parse(const std::string& fn)
{
    std::ifstream file;
    khLockGuard guard(xmlParmsLock);

    if (options.size() == 0) throw OptionsEmptyException(); // nothing to search on, exit

    file.open(fn.c_str());
    if (file.fail()) 
    {
        throw FileNotPresentException(fn);
    }
    parse(file);
    file.close();
}

const std::string& khConfigFileParser::operator[](const std::string& key) { return at(key); }
const std::string& khConfigFileParser::at(const std::string& key)
{
    auto _key(key);
    std::transform(_key.begin(), _key.end(),
                   _key.begin(), ::toupper);
    for (const auto& i : contents)
    {
        if (i.first == _key)
        {
            return i.second;
        }
    }
    throw KeyNotPresentException(key);
}
