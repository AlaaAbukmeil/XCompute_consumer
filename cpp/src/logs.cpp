// utils.cpp
#include "utils.h"
#include <ctime>

// Static member initialization
ofstream Logger::logFile;
ofstream Logger::logTradeFile;

void initializeLogging()
{
    Logger::initialize();
}

string getCurrentTimestamp()
{
    time_t now = time(0);
    char timestamp[26];
    ctime_r(&now, timestamp);
    string ts(timestamp);
    return ts.substr(0, ts.length() - 1);
}

void logToFile(const string &message)
{
    Logger::log(message);
}
void logToTradeFile(const string &message)
{
    Logger::logTrade(message);
}


// Logger class implementation
void Logger::initialize()
{
    if (!logFile.is_open())
    {
        logFile.open("./cpp/log.txt", ios::app);
    }
       if (!logTradeFile.is_open())
    {
        logTradeFile.open("./cpp/trade.txt", ios::app);
    }
}

void Logger::log(const string &message)
{
    if (!logFile.is_open())
    {
        initialize();
    }
    // logFile << "[" << getCurrentTimestamp() << "] " << message << endl;
    logFile << message << endl;
    logFile.flush();
}
void Logger::logTrade(const string &message)
{
    if (!logTradeFile.is_open())
    {
        initialize();
    }
    // logFile << "[" << getCurrentTimestamp() << "] " << message << endl;
    logTradeFile << message << endl;
    logTradeFile.flush();
}

void Logger::cleanup()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
     if (logTradeFile.is_open())
    {
        logTradeFile.close();
    }
}