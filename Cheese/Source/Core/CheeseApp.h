#ifndef CORE_CHEESE_APP_H
#define CORE_CHEESE_APP_H
#include "CoreMinimal.h"
#include "CheeseWindow.h"

class CheeseApp
{
 public:
  CheeseApp()          = default;
  virtual ~CheeseApp() = default;

  virtual bool Init() = 0;
  virtual void Exit() = 0;

  virtual bool Load()   = 0;
  virtual void UnLoad() = 0;

  virtual void Run()            = 0;
  virtual void Clear()          = 0;
  virtual void Update(float dt) = 0;

  virtual CheeseWindow* GetWindow() const = 0;
  virtual CheString GetName() const       = 0;
};

#define DEFINE_APPLICATION_MAIN(AppClass)                        \
  extern int WindowsMain(int argc, char** argv, CheeseApp* app); \
  int main(int argc, char** argv)                                \
  {                                                              \
    AppClass app;                                                \
    return WindowsMain(argc, argv, &app);                        \
  }

#endif  //  CORE_CHEESE_APP_H