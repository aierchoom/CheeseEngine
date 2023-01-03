#ifndef INPUT_INPUT_COMPONENT_H
#define INPUT_INPUT_COMPONENT_H

#include "InputHelper.h"

#include <vector>
#include <unordered_map>
#include <windows.h>
#include "Core/Delegate.hpp"

class IEvent
{
 public:
  virtual void OnKeyDown(EKeyMap keyMap) {}
  virtual void OnResize(uint32 width, uint32 height) {}
};

class InputComponent
{
 public:
  static InputComponent* Get();
  static void Clear();

  LRESULT Response(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  void RegisterOnKeyDown(IEvent& caller)
  {
    auto delegate = MakeDelegate(caller, &IEvent::OnKeyDown);
    mKeyEventMap.push_back(delegate);
  }

  void RegisterOnResize(IEvent& caller)
  {
    auto delegate = MakeDelegate(caller, &IEvent::OnResize);
    mResizeEventMap.push_back(delegate);
  }

  EKeyStatus GetKeyStatus(EKeyMap key)
  {
    // if first get,init it.
    if (mKeyStatus.find(key) == mKeyStatus.end()) {
      mKeyStatus[key] = EKeyStatus::RELEASED;
    }
    return mKeyStatus[key];
  }

 private:
  static InputComponent* mInputComponent;
  std::vector<Delegate<decltype(&IEvent::OnKeyDown)>> mKeyEventMap;
  std::vector<Delegate<decltype(&IEvent::OnResize)>> mResizeEventMap;

  std::unordered_map<EKeyMap, EKeyStatus> mKeyStatus;
};
#endif  // INPUT_INPUT_COMPONENT_H