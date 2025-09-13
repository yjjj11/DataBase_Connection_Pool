#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <queue>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <format>
using namespace std;

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

template<typename T>
class blockQueue
{
public:
    blockQueue() = default;
    ~blockQueue() = default;

    void push(const T& t)
    {
        lock_guard<mutex> locker(mtx_);
        queue_.push(t);
        notEmpty_.notify_one();
    }

    void push(T&& t)
    {
        lock_guard<mutex> locker(mtx_);
        queue_.push(std::move(t));
        notEmpty_.notify_one();
    }

    bool pop(T& t)
    {
        unique_lock<mutex> lock(mtx_);
        notEmpty_.wait(lock, [this]() {
            return !queue_.empty() || nonBlock_;
            });

        if (queue_.empty()) return false;
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void setNonBlock() {
        lock_guard<mutex> locker(mtx_);
        nonBlock_ = true;
        notEmpty_.notify_all();
    }

    bool empty() {
        lock_guard<mutex> locker(mtx_);
        return queue_.empty();
    }

private:
    bool nonBlock_ = false;
    queue<T> queue_;
    condition_variable notEmpty_;
    mutex mtx_;
};

class Logger
{
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void setLogLevel(LogLevel level) {
        lock_guard<mutex> locker(levelMtx_);
        filterLevel_ = level;
    }

    void writeLog(LogLevel level, const string& content) {
        {
            lock_guard<mutex> locker(levelMtx_);
            if (level < filterLevel_) {
                return;
            }
        }

        string logMsg = getTimeStamp() + " " + getLevelStr(level) + " " + content;
        queue_.push(std::move(logMsg));
    }

    ~Logger() {
        queue_.setNonBlock();
        if (writeThread_ && writeThread_->joinable()) {
            writeThread_->join();
            delete writeThread_;
            writeThread_ = nullptr;
        }
    }

private:
    Logger()
        : filterLevel_(LogLevel::DEBUG)
    {
        writeThread_ = new thread(&Logger::work, this);
    }

    void work() {
        while (true) {
            string logMsg;
            if (!queue_.pop(logMsg)) {
                break;
            }

            lock_guard<mutex> locker(fileMtx_);
            ofstream ofs(logPath_, ios::app);
            if (ofs.is_open()) {
                ofs << logMsg << endl;
                ofs.close();
            }
            else {
                cerr << "[Logger Error] Failed to open log file: " << logPath_ << endl;
            }
        }
    }

    string getTimeStamp() {
        auto now = chrono::system_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
        auto timeT = chrono::system_clock::to_time_t(now);

        stringstream ss;
        ss << "[" << put_time(localtime(&timeT), "%Y-%m-%d %H:%M:%S")
            << "." << setw(3) << setfill('0') << ms.count() << "]";
        return ss.str();
    }

    string getLevelStr(LogLevel level) {
        switch (level) {
        case LogLevel::DEBUG:   return "[DEBUG]";
        case LogLevel::INFO:    return "[INFO]";
        case LogLevel::WARNING: return "[WARNING]";
        case LogLevel::ERROR:   return "[ERROR]";
        case LogLevel::FATAL:   return "[FATAL]";
        default:                return "[UNKNOWN]";
        }
    }

private:
    blockQueue<string> queue_;
    thread* writeThread_ = nullptr;
    mutex fileMtx_;
    const string logPath_ = "app.log";
    LogLevel filterLevel_;
    mutex levelMtx_;
};

#define LOG_DEBUG(fmt, ...)  Logger::getInstance().writeLog(LogLevel::DEBUG,   std::format(fmt, ##__VA_ARGS__))
#define LOG_INFO(fmt, ...)   Logger::getInstance().writeLog(LogLevel::INFO,    std::format(fmt, ##__VA_ARGS__))
#define LOG_WARNING(fmt, ...)Logger::getInstance().writeLog(LogLevel::WARNING, std::format(fmt, ##__VA_ARGS__))
#define LOG_ERROR(fmt, ...)  Logger::getInstance().writeLog(LogLevel::ERROR,   std::format(fmt, ##__VA_ARGS__))
#define LOG_FATAL(fmt, ...)  Logger::getInstance().writeLog(LogLevel::FATAL,   std::format(fmt, ##__VA_ARGS__))