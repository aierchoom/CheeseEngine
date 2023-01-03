#include "Core/CheeseApp.h"
#include "Input/InputConponent.h"
#include "Graphics/D3DUtil.h"
#include "Utils/Log/Logger.h"
#include "Utils/Log/ConsoleLogDevice.h"

bool gWindowClassInilialized = false;
HINSTANCE gInstance          = NULL;

LRESULT Response(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  return InputComponent::Get()->Response(hwnd, msg, wparam, lparam);
}

void InitWindowsClass()
{
  if (!gWindowClassInilialized) {
    gInstance = (HINSTANCE)GetModuleHandle(NULL);
    WNDCLASS wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = Response;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = gInstance;
    wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName  = 0;
    wc.lpszClassName = CHEESE_WINDOW_CLASS;

    bool success = RegisterClass(&wc);

    if (!success) {
      DWORD errorMessageID = ::GetLastError();
      if (errorMessageID != ERROR_CLASS_ALREADY_EXISTS) {
        CheChar* errorMessage = nullptr;
        uint32 size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                    NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, 0, NULL);

        size += 1;

        CheChar* message = new CheChar[size];
        lstrcpy(message, errorMessage);
        MessageBox(NULL, message, CTEXT("EROOR"), MB_OK);
        delete[] message;
        return;
      } else {
        gWindowClassInilialized = false;
      }
    }
  }
}

void OpenWindow(const CheString& caption, CheeseWindow* window)
{
  window->SetCaption(caption);

  // Compute window rectangle dimensions based on requested client area demensions.
  RECT rect = {0, 0, static_cast<LONG>(window->GetWidth()), static_cast<LONG>(window->GetHeight())};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
  int width  = rect.right - rect.left;
  int height = rect.bottom - rect.top;

  HWND hwnd = CreateWindow(CHEESE_WINDOW_CLASS, window->GetCaption().c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                           width, height, 0, 0, gInstance, 0);

  window->SetHwnd(hwnd);
  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);
}

int WindowsMain(int argc, char** argv, CheeseApp* app)
{
  InitWindowsClass();
  OpenWindow(app->GetName(), app->GetWindow());
  logger.SetLogDevice(new ConsoleLogDevice());
  logger.Debug(CTEXT("Logger init!"));
  try {
    app->Init();
    app->Load();
    app->Run();
  } catch (DxException& e) {
    MessageBox(NULL, e.ToString().c_str(), CTEXT("HR Failed"), MB_OK);
    return -1;
  }
  return 0;
}