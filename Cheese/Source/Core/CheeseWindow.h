#ifndef CORE_CHEESE_WINDOW
#define CORE_CHEESE_WINDOW

#include <Windows.h>
#include "CoreMinimal.h"

class CheeseWindow
{
 public:
  CheeseWindow(uint32 width, uint32 height);
  ~CheeseWindow() = default;

  inline HWND GetHwnd() const { return mHwnd; }
  inline uint32 GetWidth() const { return mWidth; }
  inline uint32 GetHeight() const { return mHeight; }
  inline CheString GetCaption() const { return mCaption; }
  inline float GetAspectRatio() const { return static_cast<float>(mWidth) / mHeight; }

  inline void SetHwnd(HWND hwnd) { mHwnd = hwnd; }
  inline void SetWidth(uint32 width) { mWidth = width; }
  inline void SetHeight(uint32 height) { mHeight = height; }
  inline void SetCaption(const CheString& caption) { mCaption = caption; }

  inline void ReSize(uint32 width, uint32 height)
  {
    mWidth  = width;
    mHeight = height;
  }

 private:
  HWND mHwnd;
  uint32 mWidth;
  uint32 mHeight;
  CheString mCaption;
};
#endif  // CORE_CHEESE_WINDOW