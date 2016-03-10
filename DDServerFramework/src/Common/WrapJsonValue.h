#ifndef _WRAP_JSONVALUE_H
#define _WRAP_JSONVALUE_H

#include <memory>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

class WrapJsonValue
{
public:
    WrapJsonValue(rapidjson::Type type) : mValue(type)
    {
        mAllocator = std::make_shared<rapidjson::MemoryPoolAllocator < >>();
    }

    WrapJsonValue(rapidjson::Type type, std::shared_ptr<rapidjson::MemoryPoolAllocator<>> allocator) : mValue(type)
    {
        mAllocator = allocator;
    }

    std::shared_ptr<rapidjson::MemoryPoolAllocator<>>   getAllocator()
    {
        return mAllocator;
    }
    void                                                eraseMember(const std::string &key)
    {
        mValue.EraseMember(mValue.FindMember(rapidjson::Value(key.c_str(), *(mAllocator.get()))));
    }
    void                                                AddMember(const std::string& key, double value)
    {
        mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), value, *(mAllocator.get()));
    }
    void                                                AddMember(const std::string& key, bool value)
    {
        mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), value, *(mAllocator.get()));
    }
    void                                                AddMember(const std::string &key, int value)
    {
        mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), value, *(mAllocator.get()));
    }
    void                                                AddMember(const std::string &key, int64_t value)
    {
        mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), value, *(mAllocator.get()));
    }
    void                                                AddMember(const std::string &key, uint64_t value)
    {
        mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), value, *(mAllocator.get()));
    }
    void                                                AddMember(const std::string &key, const std::string& value)
    {
        mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), rapidjson::Value(value.c_str(), *(mAllocator.get())), *(mAllocator.get()));
    }
    void                                                AddMember(const std::string &key, const char* value)
    {
        mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), rapidjson::Value(value, *(mAllocator.get())), *(mAllocator.get()));
    }
    void                                                AddMember(const std::string &key, WrapJsonValue& value)
    {
        assert(this->mAllocator == value.mAllocator);
        if (this->mAllocator == value.mAllocator)
        {
            mValue.AddMember(rapidjson::Value(key.c_str(), *(mAllocator.get())), value.mValue, *(mAllocator.get()));
        }
    }

    void                                                PushBack(rapidjson::Value& value)
    {
        mValue.PushBack(value, *(mAllocator.get()));
    }
    void                                                PushBack(size_t value)
    {
        mValue.PushBack(rapidjson::Value(value), *(mAllocator.get()));
    }
    void                                                PushBack(int value)
    {
        mValue.PushBack(rapidjson::Value(value), *(mAllocator.get()));
    }

    void                                                PushBack(int64_t value)
    {
        mValue.PushBack(rapidjson::Value(value), *(mAllocator.get()));
    }

    void                                                PushBack(WrapJsonValue& value)
    {
        assert(this->mAllocator == value.mAllocator);
        if (this->mAllocator == value.mAllocator)
        {
            mValue.PushBack(value.mValue, *(mAllocator.get()));
        }
    }

    std::string                                         toString()
    {
        rapidjson::StringBuffer                     buffer;
        rapidjson::Writer<rapidjson::StringBuffer>  writer(buffer);
        mValue.Accept(writer);

        return buffer.GetString();
    }

    rapidjson::Value&                                   getValue()
    {
        return mValue;
    }
private:
    rapidjson::Value                                    mValue;
    std::shared_ptr<rapidjson::MemoryPoolAllocator<>>   mAllocator;
};

#endif