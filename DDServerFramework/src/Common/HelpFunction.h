#ifndef _HELP_FUNCTION_H
#define _HELP_FUNCTION_H

#include <iostream>
#include <string>
#include <map>
#include <exception>

template<typename K, typename V>
static typename std::map<K, V>::mapped_type& map_at(std::map<K, V>& m, K k)
{
    auto it = m.find(k);
    if (it == m.end())
    {
        std::string e = "not found key :" + k;
        throw std::runtime_error(e.c_str());
    }

    return it->second;
}

template<typename K, typename V>
static const typename std::map<K, V>::mapped_type& map_at(const std::map<K, V>& m, K k)
{
    auto it = m.find(k);
    if (it == m.end())
    {
        std::string e = "not found key :" + k;
        throw std::runtime_error(e.c_str());
    }

    return it->second;
}

static void errorExit(std::string error)
{
    std::cout << "error:" << error << std::endl;
    std::cout << "enter any key exit!" << std::endl;
    std::cin.get();
    exit(-1);
}

static int StrToInt(const char* str)
{
    return atoi(str);
}

static int64_t StrToInt64(const char* str)
{
    int64_t v;
    sscanf(str, "%lld", &v);
    return v;
}

#endif