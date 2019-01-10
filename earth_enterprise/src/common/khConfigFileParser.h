#ifndef __khConfigFileParser_h
#define __khConfigFileParser_h

#include <string>
#include <vector>
#include <map>
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

class khConfigFileParser
{
private:
    std::vector<std::string> options;
    std::map<std::string,std::string> contents;
    
    bool sanitize(std::string&);
    void split(const std::string&, std::string&, std::string&);
    bool isKeyPresent(const std::string&);
    
public:
    
    void addOption(std::string);
    void parse(const std::string&);
    
    // allow for normal looping
    int size()   { return contents.size();   }
    const std::string& at(const std::string&);
    const std::string& operator[](const std::string&);
    
    // allow for range looping
    std::map<std::string,std::string>::const_iterator begin() { return contents.begin(); }
    std::map<std::string,std::string>::const_iterator end() { return contents.end(); }	
    
    // delete everything but constructor/destructor
    khConfigFileParser()                                     = default;
	~khConfigFileParser()									 = default;

    khConfigFileParser(const khConfigFileParser&)            = delete;
    khConfigFileParser(khConfigFileParser&&)                 = delete;
    khConfigFileParser& operator=(const khConfigFileParser&) = delete;
    khConfigFileParser& operator=(khConfigFileParser&&)      = delete;
};

#endif // KHPARSECONFIGFILE_H
