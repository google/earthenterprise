#include "khConfigFileParser.h"
#include <algorithm>
#include <regex>
#include <fstream>
#include "common/khThread.h"

FileNotPresentException::FileNotPresentException(const std::string& fn)
{
    str = fn + " is not present";
}

OptionsEmptyException::OptionsEmptyException()
{
	str = "no list of options present";
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
    

bool khConfigFileParser::sanitize(std::string& line)
{
    // remove whitespace
    line = std::regex_replace(line, std::regex("\\s+"),"");
    
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
    std::regex eq("=");
    std::vector<std::string> splitLine
    {
        std::sregex_token_iterator(line.begin(),line.end(),eq,-1),
        {}
    };
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

void khConfigFileParser::parse(const std::string& fn)
{
	std::ifstream file;
	khLockGuard guard(xmlParmsLock);

	contents.clear(); //clear out the old contents	
	if (options.size() == 0) throw OptionsEmptyException(); // nothing to search on, exit

    file.open(fn.c_str());
    if (file.fail()) 
    {
        throw FileNotPresentException(fn);
    }
    
    std::string line;
    while (getline(file,line))
    {
        if (sanitize(line))
        {
            std::string key, value;
            split(line,key,value);
            if (!isKeyPresent(key)) 
            {
				file.close();
                throw KeyNotPresentException(key);
            }
            if (value.size() == 0)
            {
				file.close();
                throw ValueNotPresentException(key);
            }
            contents.insert(std::pair<std::string,std::string>(key,value));
        }
    }
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
