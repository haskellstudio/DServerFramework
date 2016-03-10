#ifndef _MAKE_UID_H
#define _MAKE_UID_H

#include <stdint.h>
#include <time.h>

union COMMON_UID
{
    int64_t id;
    struct
    {
        int32_t incID;
        int32_t time;
    }humman;
};

static int64_t CommonMakeUID()
{
    static_assert(sizeof(union COMMON_UID) == sizeof(((COMMON_UID*)nullptr)->id), "");

    static int32_t incID = 0;
    incID++;
    COMMON_UID tmp;
    tmp.humman.incID = incID;
    tmp.humman.time = static_cast<int32_t>(time(nullptr));

    return tmp.id;
}

#endif