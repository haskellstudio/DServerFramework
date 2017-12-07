#ifndef _MY_LOG_H
#define _MY_LOG_H

#include <string>
#include <iostream>
#include <thread>
#include <memory>

#include <brynet/utils/MsgQueue.h>
#include "spdlog/spdlog.h"

/*TODO::console和硬盘日志分开--因为console必须使用异步且可丢弃*/
class WrapLog
{
public:
    typedef std::shared_ptr<WrapLog> PTR;

    struct ThreadLog 
    {
        spdlog::level::level_enum type;
        std::shared_ptr<std::string> msg;
        ThreadLog()
        {}
        ThreadLog(spdlog::level::level_enum aType, const std::shared_ptr<std::string>& aMsg) : type(aType), msg(aMsg)
        {
        }
        ThreadLog(spdlog::level::level_enum aType, std::shared_ptr<std::string>&& aMsg) : type(aType), msg(std::move(aMsg))
        {
        }

        ThreadLog(const ThreadLog& right) : type(right.type), msg(right.msg)
        {}
        ThreadLog(ThreadLog&& right) : type(right.type), msg(std::move(right.msg))
        {}

        struct ThreadLog& operator =(struct ThreadLog&& right)
        {
            type = right.type;
            msg = std::move(right.msg);
            return *this;
        }
    };

    enum ConsoleAttribute
    {
        CONSOLE_INTENSE,
        CONSOLE_NO_INTENSE,
        CONSOLE_BLACK,
        CONSOLE_WHITE,
        CONSOLE_RED,
        CONSOLE_GREEN,
        CONSOLE_BLUE,
        CONSOLE_YELLOW,
        CONSOLE_MAGENTA,
        CONSOLE_CYAN
    };

public:
    WrapLog()
    {
#ifdef _WIN32

        _handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
        mCurrentColor = CONSOLE_INTENSE;

        setColor(CONSOLE_GREEN);
        setLevel(spdlog::level::debug);
        mIsClose = false;
    }

    ~WrapLog()
    {
        stop();
    }

    void                            stop()
    {
        mIsClose = true;
        if (mLogThread.joinable())
        {
            mLogThread.join();
        }
    }

    void                            setFile(std::string name, std::string fileName)
    {
        if (!mLogThread.joinable())
        {
            mConsoleLogger = spdlog::stdout_logger_st("console");
            mDiskLogger = spdlog::daily_logger_st("file_logger", fileName, 0, 0, false);

            mLogThread = std::thread([this](){
                ThreadLog tmp;
                while (!mIsClose)
                {
                    mLogQueue.syncRead(std::chrono::microseconds(5));
                    const int maxConsole = 100;
                    try
                    {
                        while (mLogQueue.popFront(tmp))
                        {
                            //全部写硬盘
                            outputDisk(tmp);
                            if (mLogQueue.readListSize() < maxConsole)
                            {
                                //只有最近的maxConsole行日志写屏幕
                                outputConcole(tmp);
                            }
                        }
                    }
                    catch (const spdlog::spdlog_ex& ex)
                    {
                        _error(ex.what());
                    }

                    mDiskLogger->flush();
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));    //也即每一秒最多写10*100=400行屏幕日志
                }
            });
        }
    }

    void    setLevel(spdlog::level::level_enum num)
    {
        mLevel = num;
    }

    template <typename... Args> void debug(const char* fmt, const Args&... args)
    {
        pushLog(spdlog::level::debug, fmt, std::forward<const Args&>(args)...);
    }

    template <typename... Args> void info(const char* fmt, const Args&... args)
    {
        pushLog(spdlog::level::info, fmt, std::forward<const Args&>(args)...);
    }
    template <typename... Args> void warn(const char* fmt, const Args&... args)
    {
        pushLog(spdlog::level::warn, fmt, std::forward<const Args&>(args)...);
    }
    template <typename... Args> void error(const char* fmt, const Args&... args)
    {
        pushLog(spdlog::level::err, fmt, std::forward<const Args&>(args)...);
    }

private:
    template <typename... Args> void    pushLog(spdlog::level::level_enum type, const char* fmt, const Args&... args)
    {
        if (shouldLog(type))
        {
            try
            {
                std::string&& str = fmt::format(fmt, std::forward<const Args&>(args)...);
                mQuequLock.lock();
                mLogQueue.push(ThreadLog(type, std::make_shared<std::string>(std::move(str))));
                mLogQueue.forceSyncWrite();
                mQuequLock.unlock();
            }
            catch (const fmt::FormatError& e)
            {
                std::string tmp = fmt;
                tmp.append(" format error:");
                tmp.append(e.what());
                mQuequLock.lock();
                mLogQueue.push(ThreadLog(spdlog::level::err, std::make_shared<std::string>(tmp)));
                mLogQueue.forceSyncWrite();
                mQuequLock.unlock();
            }
        }
    }

    void                            _error(const char* what)
    {
        setColor(CONSOLE_RED);
        try
        {
            mConsoleLogger->error("{}", what);
            mDiskLogger->error("{}", what);
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cout << ex.what() << std::endl;
        }
    }

private:
    void                            setColor(ConsoleAttribute ca)
    {
#ifdef _WIN32
        if (ca != mCurrentColor)
        {
            WORD word(colorToWORD(ca));
            SetConsoleTextAttribute(_handle, word);

            mCurrentColor = ca;
        }
#endif
    }
#ifdef _WIN32
    WORD                            colorToWORD(ConsoleAttribute ca)
    {
        CONSOLE_SCREEN_BUFFER_INFO screen_infos;
        WORD word, flag_instensity;
        GetConsoleScreenBufferInfo(_handle, &screen_infos);

        word = screen_infos.wAttributes;

        flag_instensity = word & FOREGROUND_INTENSITY;

        switch (ca)
        {
        case CONSOLE_BLACK:
            word = 0;
            break;
        case CONSOLE_WHITE:
            word = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;
        case CONSOLE_RED:
            word = FOREGROUND_RED;
            break;
        case CONSOLE_GREEN:
            word = FOREGROUND_GREEN;
            break;
        case CONSOLE_BLUE:
            word = FOREGROUND_BLUE;
            break;
        case CONSOLE_YELLOW:
            word = FOREGROUND_RED | FOREGROUND_GREEN;
            break;
        case CONSOLE_MAGENTA:
            word = FOREGROUND_RED | FOREGROUND_BLUE;
            break;
        case CONSOLE_CYAN:
            word = FOREGROUND_GREEN | FOREGROUND_BLUE;
            break;
        }

        word |= flag_instensity;

        return word;
    }
#endif

    bool    shouldLog(spdlog::level::level_enum num) const
    {
        return num >= mLevel;
    }

    void    outputConcole(ThreadLog& log)
    {
        switch (log.type)
        {
        case spdlog::level::debug:
            setColor(CONSOLE_CYAN);
            break;
        case  spdlog::level::info:
            setColor(CONSOLE_GREEN);
            break;
        case spdlog::level::warn:
            setColor(CONSOLE_YELLOW);
            break;
        case spdlog::level::err:
            setColor(CONSOLE_RED);
            break;
        default:
            break;
        }

        mConsoleLogger->force_log(log.type, log.msg->c_str());
    }

    void    outputDisk(ThreadLog& log)
    {
        mDiskLogger->force_log(log.type, log.msg->c_str());
    }
private:
    brynet::MsgQueue<ThreadLog>       mLogQueue;

    std::shared_ptr<spdlog::logger> mConsoleLogger; /*TODO::可以改为异步,不会阻塞硬盘日志的写入*/
    std::shared_ptr<spdlog::logger> mDiskLogger;
    spdlog::level::level_enum       mLevel;

#ifdef _WIN32
    HANDLE                          _handle;
#endif

    ConsoleAttribute                mCurrentColor;
    std::mutex                      mQuequLock;
    std::thread                     mLogThread;
    bool                            mIsClose;
};

#endif