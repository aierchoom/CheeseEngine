#include "ConsoleLogDevice.h"
#include <iostream>

#include "Utils/Date/Date.h"

// Cyan
#define DEBUG_COLOR "\033[36m"
// White
#define INFO_COLOR "\033[37m"
// Yellow
#define WARNING_COLOR "\033[33m"
// Red
#define ERROR_COLOR "\033[31m"

using namespace std;

#ifdef UNICODE
auto& out = wcout;
#else
auto& out = cout;
#endif

void ConsoleLogDevice::DebugMsg(const CheString& info)
{
  out << DEBUG_COLOR;
  out.width(11);
  out << left << CTEXT("[Debug]:");
  out << Date::Now().ToString() << CTEXT(":");
  out << info << endl;
}

void ConsoleLogDevice::InfoMsg(const CheString& info)
{
  out << INFO_COLOR;
  out.width(11);
  out << left << CTEXT("[Info]:");
  out << Date::Now().ToString() << CTEXT(":");
  out << info << endl;
}

void ConsoleLogDevice::WarningMsg(const CheString& info)
{
  out << WARNING_COLOR;
  out.width(11);
  out << CTEXT("[Warning]:");
  out << Date::Now().ToString() << CTEXT(":");
  out << info << endl;
}

void ConsoleLogDevice::ErrorMsg(const CheString& info)
{
  out << ERROR_COLOR;
  out.width(11);
  out << left << CTEXT("[Error]:");
  out << Date::Now().ToString() << CTEXT(":");
  out << info << endl;
}