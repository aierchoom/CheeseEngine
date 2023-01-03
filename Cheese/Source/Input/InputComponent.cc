#include "InputConponent.h"

#include "Core/CoreMinimal.h"

InputComponent* InputComponent::mInputComponent = new InputComponent();

InputComponent* InputComponent::Get() { return mInputComponent; }

void InputComponent::Clear() { SAFE_RELEASE_PTR(mInputComponent); }

LRESULT InputComponent::Response(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg) {
    case WM_KEYDOWN: {
      EKeyMap key     = EKeyMap(wparam - 'A');
      mKeyStatus[key] = EKeyStatus::PRESSED;
      for (auto keyEvent : mKeyEventMap) {
        keyEvent(key);
      }
      return 0;
    }
    case WM_KEYUP: {
      EKeyMap key     = EKeyMap(wparam - 'A');
      mKeyStatus[key] = EKeyStatus::RELEASED;
      return 0;
    }
    case WM_ACTIVATE:
      return 0;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_SIZE:
      uint32 width  = LOWORD(lparam);
      uint32 height = HIWORD(lparam);
      for (auto resizeEvent : mResizeEventMap) {
        resizeEvent(width, height);
      }
  }

  return DefWindowProc(hwnd, msg, wparam, lparam);
}