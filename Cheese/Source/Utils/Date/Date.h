#ifndef UTILS_DATE_DATE_H
#define UTILS_DATE_DATE_H

#include "Common/TypeDef.h"

class Date
{
 public:
  static Date Now();

  inline int32 GetYear() const { return mYear; }
  inline int32 GetMonth() const { return mMonth; }
  inline int32 GetDay() const { return mDay; }
  inline int32 GetHour() const { return mHour; }
  inline int32 GetMinute() const { return mMinute; }
  inline int32 GetSecond() const { return mSecond; }

  CheString ToString() const;

 private:
  int32 mYear;
  int32 mMonth;
  int32 mDay;

  int32 mHour;
  int32 mMinute;
  int32 mSecond;
};
#endif  // UTILS_DATE_DATE_H
