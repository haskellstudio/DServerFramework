#ifndef _MY_LOG_H
#define _MY_LOG_H

#include <string>
#include <iostream>
#include <thread>
#include <memory>

#include "msgqueue.h"
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
        _handle = GetStdHandle(STD_OUTPUT_HANDLE);
        mCurrentColor = CONSOLE_INTENSE;

        setColor(CONSOLE_GREEN);
        setLevel(spdlog::level::debug);
        mIsInit = false;
    }

    void                            setFile(std::string name, std::string fileName)
    {
        if (mIsInit)
        {
            return;
        }

        mIsInit = true;
        mConsoleLogger = spdlog::stdout_logger_st("console");
        mDiskLogger = spdlog::daily_logger_st("file_logger", fileName, 0, 0, true);

        std::thread([this](){
            while (true)
            {
                ThreadLog tmp;
                mLogQueue.SyncRead(5);
                const int maxConsole = 100;
                try
                {
                    while (mLogQueue.PopFront(&tmp))
                    {
                        //全部写硬盘

                        outputDisk(tmp);

                        if (mLogQueue.ReadListSize() < maxConsole)
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

                std::this_thread::sleep_for(std::chrono::milliseconds(100));    //也即每一秒最多写10*100=400行屏幕日志
            }
        }).detach();    /*todo::进程关闭通知并等待线程结束*/
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
        mQuequLock.lock();

        if (shouldLog(type))
        {
            try
            {
                string&& str = fmt::format(fmt, std::forward<const Args&>(args)...);
                mLogQueue.Push({ type, std::make_shared<string>(std::move(str)) });
                mLogQueue.ForceSyncWrite();
            }
            catch(const fmt::FormatError& e)
            {
                string tmp = fmt;
                tmp.append(" format error:");
                tmp.append(e.what());
                mLogQueue.Push({ spdlog::level::err, std::make_shared<string>(tmp) });
                mLogQueue.ForceSyncWrite();
            }
        }

        mQuequLock.unlock();
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
        if (ca != mCurrentColor)
        {
            WORD word(colorToWORD(ca));
            SetConsoleTextAttribute(_handle, word);

            mCurrentColor = ca;
        }
    }
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
    MsgQueue<ThreadLog>             mLogQueue;

    std::shared_ptr<spdlog::logger> mConsoleLogger; /*TODO::可以改为异步,不会阻塞硬盘日志的写入*/
    std::shared_ptr<spdlog::logger> mDiskLogger;
    spdlog::level::level_enum       mLevel;

    HANDLE                          _handle;
    ConsoleAttribute                mCurrentColor;
    bool                            mIsInit;
    std::mutex                      mQuequLock;
};

#endif