#include "Date.h"

#include <time.h>

Date Date::Now()
{
  Date date;
  time_t timep;
  time(&timep);

  tm nowTime;

  gmtime_s(&nowTime, &timep);

  date.mYear   = nowTime.tm_year + 1900;
  date.mMonth  = nowTime.tm_mon + 1;
  date.mDay    = nowTime.tm_mday;
  date.mHour   = nowTime.tm_hour + 8;
  date.mMinute = nowTime.tm_min;
  date.mSecond = nowTime.tm_sec;

  return date;
}

CheString Date::ToString() const
{
  CheChar buffer[32];
  CheSprintf(buffer, CTEXT("%04d-%02d-%02d %02d:%02d:%02d"), mYear, mMonth, mDay, mHour, mMinute, mSecond);
  return CheString(buffer);
}
