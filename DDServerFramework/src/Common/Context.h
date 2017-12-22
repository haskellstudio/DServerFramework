#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <unordered_map>
#include <string>
#include <any>

class Context
{
public:
    void    add(const std::string& key, const std::any& value)
    {
        mValues[key] = value;
    }

    std::any get(const std::string& key)
    {
        return mValues[key];
    }

    void clear()
    {
        mValues.clear();
    }
private:
    std::unordered_map<std::string, std::any> mValues;
};


#endif