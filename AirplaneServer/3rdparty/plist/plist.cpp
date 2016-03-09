#include <assert.h>
#include <iostream>
#include <time.h>
using namespace std;

#include "../pugixml/pugixml.hpp"
#include "../base64/base64.h"
#include "plist.h"

PntArray::~PntArray()
{
    for (auto& v : mValue)
    {
        delete v;
    }
    mValue.clear();
}

PntDict::~PntDict()
{
    for (auto& v : mValue)
    {
        delete v.second;
    }
    mValue.clear();
}

void PntDate::setTimeFromXMLConvention(const std::string& str)
{
    int month, day, year, hour24, minute, second;

    // parse date string.  E.g.  2011-09-25T02:31:04Z
    sscanf(str.c_str(), "%4d-%2d-%2dT%2d:%2d:%2dZ", &year, &month, &day, &hour24, &minute, &second);
    struct tm tmTime;
    tmTime.tm_hour = hour24;
    tmTime.tm_mday = day;
    tmTime.tm_year = year - 1900;
    tmTime.tm_sec = second;
    tmTime.tm_mon = month - 1;
    tmTime.tm_min = minute;

    //get proper day light savings.

    time_t loc = time(NULL);
    struct tm tmLoc = *localtime(&loc);
    //std::cout<<"tmLoc.tm_isdst = "<<tmLoc.tm_isdst<<std::endl;
    tmTime.tm_isdst = tmLoc.tm_isdst;

    if (true)
    {
        //mTime = timegm(&tmTime);

        tmTime.tm_isdst = 0;
        mTime = mktime(&tmTime);
        if (mTime < -1)
            throw PlistParseError("Plist::Date::set() date invalid");

        // don't have timegm for all systems so here's a portable way to do it.

        struct tm tmTimeTemp;
#if defined(_WIN32) || defined(_WIN64)
        gmtime_s(&tmTimeTemp, &mTime);
#else
        gmtime_r(&mTime, &tmTimeTemp);
#endif

        time_t timeTemp = mktime(&tmTimeTemp);

        time_t diff = mTime - timeTemp;
        mTime += diff;
    }
    else
    {
        mTime = mktime(&tmTime);
        if (mTime < -1)
            throw PlistParseError("Plist::Date::set() date invalid");
    }
}

static AnyValue* parseAny(pugi::xml_node& node);
static AnyValue* parseDict(pugi::xml_node&);
static AnyValue* parseArray(pugi::xml_node&);
static AnyValue* parseDate(pugi::xml_node&);

static string load_file(const char *filename, bool& loadResult)
{
    loadResult = false;
    string content;

    FILE *fp = fopen(filename, "rb");
    if (fp != nullptr)
    {
        char* buffer = NULL;
        int file_size = 0;

        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buffer = (char*)malloc(file_size + 1);
        fread(buffer, file_size, 1, fp);
        buffer[file_size] = 0;
        fclose(fp);

        content = string(buffer, file_size + 1);
        loadResult = true;
    }

    return content;
}

static AnyValue* parseAny(pugi::xml_node& node)
{
    AnyValue* ret = nullptr;
    
    using namespace std;

    string nodeName = node.name();

    if ("dict" == nodeName)
        ret = parseDict(node);
    else if ("array" == nodeName)
        ret = parseArray(node);
    else if ("string" == nodeName)
        ret = new PntString(string(node.first_child().value()));
    else if ("integer" == nodeName)
        ret = new PntInt((int64_t)atoll(node.first_child().value()));
    else if ("real" == nodeName)
        ret = new PntReal(atof(node.first_child().value()));
    else if ("false" == nodeName)
        ret = new PntBool(false);
    else if ("true" == nodeName)
        ret = new PntBool(true);
    else if ("data" == nodeName)
        ret = new PntString(base64_decode(node.first_child().value()));    /*  TODO::base64 decode*/
    else if ("date" == nodeName)
        ret = parseDate(node);
    else
        throw PlistParseError(string("Plist: XML unknown node type " + nodeName));

    return ret;
}

static AnyValue* parseArray(pugi::xml_node& node)
{
    using namespace std;

    PntArray* array = new PntArray;
    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
        array->addNode(parseAny(*it));

    return array;
}

static AnyValue* parseDict(pugi::xml_node& node)
{
    using namespace std;

    PntDict* dict = new PntDict;
    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
    {
        if (string("key") != it->name())
        {
            delete dict;
            throw PlistParseError("Plist: XML dictionary key expected but not found");
        }

        string key(it->first_child().value());
        ++it;

        if (it == node.end())
        {
            delete dict;
            throw PlistParseError("Plist: XML dictionary value expected for key " + key + "but not found");
        }
        else if (string("key") == it->name())
        {
            delete dict;
            throw PlistParseError("Plist: XML dictionary value expected for key " + key + "but found another key node");
        }

        dict->addNode(key, parseAny(*it));
    }

    return dict;
}

static AnyValue* parseDate(pugi::xml_node& node)
{
    AnyValue* ret = new PntDate(node.first_child().value());         /*  TODO::decode date   */
    return ret;
}

AnyValue* parsePlist(const std::string& source)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(source.c_str(), (size_t)source.size());
    if (!result)
        throw PlistParseError((string("Plist: XML parsed with error ") + result.description()).c_str());

    pugi::xml_node rootNode = doc.child("plist").first_child();
    return parseAny(rootNode);
}

AnyValue* parsePlistByFileName(const std::string& fileName)
{
    bool loadResult = false;
    string source = load_file(fileName.c_str(), loadResult);
    if (!loadResult)
    {
        throw PlistParseError((string("Plist: File" + fileName + " Load Failed")));
        return nullptr;
    }

    return parsePlist(source);
}