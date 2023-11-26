#pragma once
#define NO_COPY(typeName)                        \
  typeName(const typeName&)            = delete; \
  typeName& operator=(const typeName&) = delete;

#define NO_MOVE(typeName)                         \
  typeName(const typeName&&)            = delete; \
  typeName& operator=(const typeName&&) = delete;
