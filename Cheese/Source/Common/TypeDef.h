#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#include <stdint.h>

using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8  = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using Byte = uint8;

#include <windows.h>

#include <string>

#ifdef _UNICODE
using CheChar   = wchar_t;
using CheString = std::wstring;
inline CheString ConvertToCheString(int num) { return std::to_wstring(num); }
inline CheString ConvertToCheString(const char* str)
{
  CheChar buffer[512];
  MultiByteToWideChar(0, 0, str, -1, buffer, 512);
  return CheString(buffer);
}
inline std::string ConvertToMultiByte(const CheString& str)
{
  std::string multiByteStr;
  char* buffer = new char[str.size() + 1];
  WideCharToMultiByte(0, 0, str.c_str(), -1, buffer, static_cast<uint32>(str.size()), 0, 0);
  buffer[str.size()] = '\0';
  multiByteStr       = buffer;
  if (buffer != nullptr) {
    delete[] buffer;
    buffer = nullptr;
  }
  return multiByteStr;
}
inline std::wstring ConvertToWideByte(const CheString& str) { return CheString(str); }

#define CheSprintf wsprintf

#define CTEXT(x) L##x
#else
using CheChar   = char;
using CheString = std::string;
inline CheString ConvertToCheString(int num) { return std::to_string(num); }
inline CheString ConvertToCheString(const CheChar* str) { return CheString(str); }
inline std::string ConvertToMultiByte(const CheString& str) { return CheString(str); }
inline std::wstring ConvertToWideByte(const CheString& str)
{
  std::wstring wideByteStr;
  wchar_t* buffer = new wchar_t[str.size()];
  MultiByteToWideChar(0, 0, str.c_str(), -1, buffer, static_cast<int32>(str.size()));
  wideByteStr = buffer;
  if (buffer != nullptr) {
    delete[] buffer;
    buffer = nullptr;
  }
  return wideByteStr;
}

#define CheSprintf sprintf;

#define CTEXT(x) x
#endif

#include <wrl/client.h>
template <class T>

using ComPtr = Microsoft::WRL::ComPtr<T>;

#endif  // COMMON_DEF_H