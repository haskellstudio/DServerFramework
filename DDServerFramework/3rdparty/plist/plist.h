#ifndef _PLIST_H
#define _PLIST_H

#include <vector>
#include <map>
#include <stdint.h>
#include <exception>
#include <string>

enum PLIST_NODE_TYPE
{
    PNT_INT,
    PNT_BOOL,
    PNT_STRING,
    PNT_REAL,
    PNT_DATE,
    PNT_ARRAY,
    PNT_DICT,
};

class PntInt;
class PntReal;
class PntBool;
class PntDate;
class PntString;
class PntArray;
class PntDict;

class PlistParseError : public std::runtime_error
{
public:
    PlistParseError(const std::string& what) : std::runtime_error(what)
    {}
};

class AnyValue
{
public:
    explicit AnyValue(PLIST_NODE_TYPE type) : mType(type){}
    virtual ~AnyValue(){}

    PLIST_NODE_TYPE     getType() const
    {
        return mType;
    }

    const PntInt*       asInt() const
    {
        if (mType != PNT_INT)
        {
            throwAsException(__FUNCTION__ + std::string(", real type is ") + std::to_string(mType));
        }

        return (const PntInt*)(this);
    }
    const PntReal*            asReal() const
    {
        if (mType != PNT_REAL)
        {
            throwAsException(__FUNCTION__ + std::string(", real type is ") + std::to_string(mType));
        }

        return  (const PntReal*)(this);
    }
    const PntBool*            asBool() const
    {
        if (mType != PNT_BOOL)
        {
            throwAsException(__FUNCTION__ + std::string(", real type is ") + std::to_string(mType));
        }

        return  (const PntBool*)(this);
    }
    const PntDate*            asDate() const
    {
        if (mType != PNT_DATE)
        {
            throwAsException(__FUNCTION__ + std::string(", real type is ") + std::to_string(mType));
        }

        return  (const PntDate*)(this);
    }
    const PntString*          asString() const
    {
        if (mType != PNT_STRING)
        {
            throwAsException(__FUNCTION__ + std::string(", real type is ") + std::to_string(mType));
        }

        return (const PntString*)(this);
    }
    const PntArray*           asArray() const
    {
        if (mType != PNT_ARRAY)
        {
            throwAsException(__FUNCTION__ + std::string(", real type is ") + std::to_string(mType));
        }

        return (const PntArray*)(this);
    }
    const PntDict*            asDict() const
    {
        if (mType != PNT_DICT)
        {
            throwAsException(__FUNCTION__ + std::string(", real type is ") + std::to_string(mType));
        }

        return (const PntDict*)(this);
    }

private:
    void    throwAsException(std::string errorInfo) const
    {
        throw std::runtime_error(errorInfo.c_str());
    }
private:
    PLIST_NODE_TYPE     mType;
    /*  TODO::使用union去掉继承关系 */
};

class PntInt : public AnyValue
{
public:
    PntInt(int64_t value) : AnyValue(PNT_INT), mValue(value){}
    int64_t             getValue() const
    {
        return mValue;
    }
private:
    int64_t             mValue;
};

class PntReal : public AnyValue
{
public:
    PntReal(double value) : AnyValue(PNT_REAL), mValue(value){}
    double              getValue() const
    {
        return mValue;
    }
private:
    double              mValue;
};

class PntBool : public AnyValue
{
public:
    PntBool(bool value) : AnyValue(PNT_BOOL), mValue(value){}
    bool                getValue() const
    {
        return mValue;
    }
private:
    bool                mValue;
};

class PntDate : public AnyValue
{
public:
    PntDate(const std::string& str) : AnyValue(PNT_DATE)
    {
        setTimeFromXMLConvention(str);
    }

    time_t              getValue() const
    {
        return mTime;
    }

private:
    void                setTimeFromXMLConvention(const std::string& str);
private:
    time_t              mTime;
};

class PntString : public AnyValue
{
public:
    PntString(const std::string& value) : AnyValue(PNT_STRING), mValue(value){}
    const std::string&  getValue() const
    {
        return mValue;
    }
private:
    std::string         mValue;
};

class PntArray : public AnyValue
{
public:
    typedef std::vector<AnyValue*> VALUE_TYPE;
    PntArray() : AnyValue(PNT_ARRAY){}
    virtual ~PntArray() override;

    void                addNode(AnyValue* value)
    {
        mValue.push_back(value);
    }

    const VALUE_TYPE&   getValue() const
    {
        return mValue;
    }

    int size(void) const
    {
        return mValue.size();
    }

    const AnyValue* operator[](int n) const
    {
        return mValue[n];
    }

private:
    VALUE_TYPE          mValue;
};

class PntDict : public AnyValue
{
public:
    typedef std::map<std::string, AnyValue*> VALUE_TYPE;
    PntDict() : AnyValue(PNT_DICT){}
    virtual ~PntDict() override;

    void                    addNode(const std::string& key, AnyValue* value)
    {
        mValue[key] = value;
    }

    const VALUE_TYPE&   getValue() const
    {
        return mValue;
    }

    const AnyValue* operator[](std::string key) const
    {
        auto it = mValue.find(key);
        if (it != mValue.end())
        {
            return it->second;
        }
        else
        {
            return nullptr;
        }
    }

    const AnyValue* findValue(std::string key) const
    {
        return (*this)[key];
    }

    //读取key的值

    int intValue(const char *key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }
        
        return  static_cast<int>(it->second->asInt()->getValue());
    }

    double realOrIntValue(const char* key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }

        return  it->second->getType() == PNT_REAL ? it->second->asReal()->getValue() : it->second->asInt()->getValue();
    }

    int intOrRealValue(const char* key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }

        return  it->second->getType() == PNT_REAL ? static_cast<int>(it->second->asReal()->getValue()) : static_cast<int>(it->second->asInt()->getValue());
    }

    double realValue(const char *key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }

        return  it->second->asReal()->getValue();
    }

    bool boolValue(const char *key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }

        return  it->second->asBool()->getValue();
    }

    const std::string& stringValue(const char *key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }

        return  it->second->asString()->getValue();
    }

    const PntArray* arrayValue(const char *key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }

        return  it->second->asArray();
    }

    const PntDict* dictValue(const char *key) const
    {
        auto it = mValue.find(key);
        if (it == mValue.end())
        {
            throwKeyNotFoundException(key, __FUNCTION__);
        }

        return  it->second->asDict();
    }

private:
    void        throwKeyNotFoundException(std::string key, std::string where) const
    {
        std::string errorInfo = std::string("can not find key : ") + key + std::string(" in ") + where;
        throw std::runtime_error(errorInfo.c_str());
    }
private:
    VALUE_TYPE          mValue;
};


AnyValue*    parsePlist(const std::string& source);
AnyValue*    parsePlistByFileName(const std::string& filename);

#endif