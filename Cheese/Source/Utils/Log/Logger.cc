#include "Logger.h"
using namespace std;

Logger logger;

void Logger::SetLogDevice(ILogDevice* device) { logger.mDevice = device; }

void Logger::Debug(const CheString& info) { mDevice->DebugMsg(info); }

void Logger::Info(const CheString& info) { mDevice->InfoMsg(info); }

void Logger::Warning(const CheString& info) { mDevice->WarningMsg(info); }

void Logger::Error(const CheString& info) { mDevice->ErrorMsg(info); }
