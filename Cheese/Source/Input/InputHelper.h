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
  KEY_Z = 25
};

enum class EKeyStatus : uint8 { RELEASED = 0, PRESSED = 1 };
#endif  // INPUT_INPUT_HELPER_H