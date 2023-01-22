#include "InputConponent.h"
#include <windowsx.h>
#include "Utils/Log/Logger.h"
#include "Core/CoreMinimal.h"

using namespace std;

InputComponent* InputComponent::mInputComponent = new InputComponent();

InputComponent* InputComponent::Get() { return mInputComponent; }

void InputComponent::Clear() { SAFE_RELEASE_PTR(mInputComponent); }

LRESULT InputComponent::Response(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
    case WM_KEYDOWN: {
      EKeyMap key = EKeyMap(wparam - 'A');
      if (mKeyStatus[key] == EKeyStatus::RELEASED) {
        mKeyStatus[key] = EKeyStatus::PRESSED;

        // process pressed event
        vector<shared_ptr<IActionEvent>> pressedEventList = mPressedMap[key];
        for (auto event : pressedEventList) {
          (*event)();
        }
      }
      return 0;
    }
    case WM_KEYUP: {
      EKeyMap key = EKeyMap(wparam - 'A');
      if (mKeyStatus[key] == EKeyStatus::PRESSED) {
        mKeyStatus[key] = EKeyStatus::RELEASED;

        // process pressed event
        vector<shared_ptr<IActionEvent>> releasedEventList = mReleasedMap[key];
        for (auto event : releasedEventList) {
          (*event)();
        }
      }
      return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN: {
      const uint32 rbdown = 0;
      const uint32 lbdown = 1;
      EKeyMap key;
      if (wparam == lbdown) {
        key = EKeyMap::MOUSE_LBUTTON;
      }
      if (wparam == rbdown) {
        key = EKeyMap::MOUSE_RBUTTON;
      }
      mKeyStatus[key] = EKeyStatus::PRESSED;

      vector<shared_ptr<IActionEvent>> pressedEventList = mPressedMap[key];
      for (auto event : pressedEventList) {
        (*event)();
      }
      return 0;
    }
    case WM_LBUTTONUP: {
      EKeyMap key = EKeyMap::MOUSE_LBUTTON;

      mKeyStatus[key] = EKeyStatus::RELEASED;

      vector<shared_ptr<IActionEvent>> releasedEventList = mReleasedMap[key];
      for (auto event : releasedEventList) {
        (*event)();
      }
      return 0;
    }
    case WM_RBUTTONUP: {
      EKeyMap key = EKeyMap::MOUSE_RBUTTON;

      mKeyStatus[key] = EKeyStatus::RELEASED;

      vector<shared_ptr<IActionEvent>> releasedEventList = mReleasedMap[key];
      for (auto event : releasedEventList) {
        (*event)();
      }
      return 0;
    }

    case WM_MOUSEMOVE: {
      if (mLastPoint.x == 0 && mLastPoint.y == 0) {
        mLastPoint.x = GET_X_LPARAM(lparam);
        mLastPoint.y = GET_Y_LPARAM(lparam);
      }

      float deltaX = GET_X_LPARAM(lparam) - mLastPoint.x;
      float deltaY = GET_Y_LPARAM(lparam) - mLastPoint.y;
      mLastPoint.x = GET_X_LPARAM(lparam);
      mLastPoint.y = GET_Y_LPARAM(lparam);
      for (auto xEvents : mAxisEventMap[EAxisEvent::MOUSE_X]) {
        (*xEvents)(-deltaX);
      }
      for (auto yEvents : mAxisEventMap[EAxisEvent::MOUSE_Y]) {
        (*yEvents)(deltaY);
      }
      return -1;
    }
    case WM_ACTIVATE:
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_SIZE:
      uint32 width  = LOWORD(lparam);
      uint32 height = HIWORD(lparam);
      for (auto resizeEvent : mResizeEventList) {
        (*resizeEvent)(width, height);
      }
  }

  return DefWindowProc(hwnd, msg, wparam, lparam);
}