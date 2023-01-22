#ifndef INPUT_INPUT_HELPER_H
#define INPUT_INPUT_HELPER_H

#include "Core/CoreMinimal.h"

enum class EInputEventType : uint8 {
  WINDOW_RESIZE,
  KEY_BROAD,
  MOUSE,
};

enum class EKeyMap : uint8 {
  KEY_A = 0,
  KEY_B = 1,
  KEY_C = 2,
  KEY_D = 3,
  KEY_E = 4,
  KEY_F = 5,
  KEY_G = 6,
  KEY_H = 7,
  KEY_I = 8,
  KEY_J = 9,
  KEY_K = 10,
  KEY_L = 11,
  KEY_M = 12,
  KEY_N = 13,
  KEY_O = 14,
  KEY_P = 15,
  KEY_Q = 16,
  KEY_R = 17,
  KEY_S = 18,
  KEY_T = 19,
  KEY_U = 20,
  KEY_V = 21,
  KEY_W = 22,
  KEY_X = 23,
  KEY_Y = 24,
  KEY_Z = 25,
  MOUSE_LBUTTON,
  MOUSE_RBUTTON,
};

enum class EAxisEvent : uint8 {
  MOUSE_X,
  MOUSE_Y,
};

struct MousePoint {
  int32 x;
  int32 y;
};

enum class EKeyStatus : uint8 { RELEASED = 0, PRESSED = 1 };

class IActionEvent
{
 public:
  virtual void operator()() = 0;
};

template <typename UserClass>
class ActionEventPtr : public IActionEvent
{
  typedef void (UserClass::*MethodType)();

 public:
  ActionEventPtr(UserClass& user, MethodType method) : mUser(user), mMethod(method) {}

  virtual void operator()() override { (mUser.*mMethod)(); }

 private:
  UserClass& mUser;
  MethodType mMethod;
};

class IResizeEvent
{
 public:
  virtual void operator()(uint32 width, uint32 height) = 0;
};

template <typename UserClass>
class ResizeEventPtr : public IResizeEvent
{
  typedef void (UserClass::*MethodType)(uint32, uint32);

 public:
  ResizeEventPtr(UserClass& user, MethodType method) : mUser(user), mMethod(method) {}

  virtual void operator()(uint32 width, uint32 height) override { (mUser.*mMethod)(width, height); }

 private:
  UserClass& mUser;
  MethodType mMethod;
};

class IAxisEvent
{
 public:
  virtual void operator()(float value) = 0;
};

template <typename UserClass>
class AxisEventPtr : public IAxisEvent
{
  typedef void (UserClass::*MethodType)(float);

 public:
  AxisEventPtr(UserClass& user, MethodType method) : mUser(user), mMethod(method) {}

  virtual void operator()(float value) override { (mUser.*mMethod)(value); }

 private:
  UserClass& mUser;
  MethodType mMethod;
};
#endif  // INPUT_INPUT_HELPER_H