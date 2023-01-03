#ifndef UTILS_LOG_ILOG_DEVICE_H
#define UTILS_LOG_ILOG_DEVICE_H
#include "Core/CoreMinimal.h"

class ILogDevice
{
 public:
  virtual ~ILogDevice() {}

  virtual void DebugMsg(const CheString& info)   = 0;
  virtual void InfoMsg(const CheString& info)    = 0;
  virtual void WarningMsg(const CheString& info) = 0;
  virtual void ErrorMsg(const CheString& info)   = 0;
};

#endif  // UTILS_LOG_ILOG_DEVICE_H
