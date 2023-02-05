#ifndef INPUT_INPUT_COMPONENT_H
#define INPUT_INPUT_COMPONENT_H

#include "InputHelper.h"

#include <vector>
#include <unordered_map>
#include <memory>
#include <windows.h>
#include "Core/Delegate.hpp"

class InputComponent
{
 public:
  static InputComponent* Get();
  static void Clear();

  LRESULT Response(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  void Tick(float dt) {}

  template <typename UserClass, typename MethodType>
  void RegisterOnAction(EKeyMap key, EKeyStatus keyStatus, UserClass& user, MethodType method)
  {
    if (keyStatus == EKeyStatus::PRESSED) {
      mPressedMap[key].push_back(std::make_shared<ActionEventPtr<UserClass>>(user, method));
    }
    if (keyStatus == EKeyStatus::RELEASED) {
      mReleasedMap[key].push_back(std::make_shared<ActionEventPtr<UserClass>>(user, method));
    }
  }

  template <typename UserClass, typename MethodType>
  void RegisterOnAxis(EAxisEvent event, UserClass& user, MethodType method)
  {
    mAxisEventMap[event].push_back(std::make_shared<AxisEventPtr<UserClass>>(user, method));
  }

  template <typename UserClass, typename MethodType>
  void RegisterOnResize(UserClass& user, MethodType method)
  {
    mResizeEventList.push_back(std::make_shared<ResizeEventPtr<UserClass>>(user, method));
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

  std::unordered_map<EKeyMap, std::vector<std::shared_ptr<IActionEvent>>> mPressedMap;
  std::unordered_map<EKeyMap, std::vector<std::shared_ptr<IActionEvent>>> mReleasedMap;
  std::unordered_map<EAxisEvent, std::vector<std::shared_ptr<IAxisEvent>>> mAxisEventMap;
  std::vector<std::shared_ptr<IResizeEvent>> mResizeEventList;
  std::unordered_map<EKeyMap, EKeyStatus> mKeyStatus;

  MousePoint mLastPoint = {0, 0};
};
#endif  // INPUT_INPUT_COMPONENT_H