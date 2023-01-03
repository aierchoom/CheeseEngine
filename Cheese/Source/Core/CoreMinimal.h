#ifndef CORE_CORE_MINIMAL_H
#define CORE_CORE_MINIMAL_H
#include "Common/TypeDef.h"

#define CHEESE_WINDOW_CLASS CTEXT("CheeseWindowClass")

#define SAFE_RELEASE_PTR(p) \
  if (p != nullptr) {       \
    delete p;               \
    p = nullptr;            \
  }

#endif  // CORE_CORE_MINIMAL_H