#ifndef UTILS_LOG_LOGGER
#define UTILS_LOG_LOGGER
#include "ILogDevice.h"
#include <memory>
#include "Common/TypeDef.h"

class Logger
{
 public:
  void Debug(const CheString& info);
  void Info(const CheString& info);
  void Warning(const CheString& info);
  void Error(const CheString& info);
  void SetLogDevice(ILogDevice* device);

 private:
  ILogDevice* mDevice = nullptr;
};

extern Logger logger;
#endif  // UTILS_LOG_LOGGER