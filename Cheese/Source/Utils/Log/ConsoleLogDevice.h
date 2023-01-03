#ifndef UTILS_LOG_CONSOLE_LOG_DEVICE
#define UTILS_LOG_CONSOLE_LOG_DEVICE
#include "ILogDevice.h"

class ConsoleLogDevice : public ILogDevice
{
 public:
  virtual ~ConsoleLogDevice() {}

  virtual void DebugMsg(const CheString& info) override;
  virtual void InfoMsg(const CheString& info) override;
  virtual void WarningMsg(const CheString& info) override;
  virtual void ErrorMsg(const CheString& info) override;
};

#endif  // UTILS_LOG_CONSOLE_LOG_DEVICE