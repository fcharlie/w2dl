////
#include "stdafx.h"
//
#include "stdio.hpp"
#include <commctrl.h>
#include <objbase.h>
#include <shobjidl.h>

class CoInitializeHelper {
public:
  CoInitializeHelper() {
    auto hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (!SUCCEEDED(hr)) {
      CoUninitialize();
      return;
    }
  }
  CoInitializeHelper(const CoInitializeHelper &) = delete;
  CoInitializeHelper &operator=(const CoInitializeHelper &) = delete;
  ~CoInitializeHelper() {
    //
    CoUninitialize();
  }

private:
};

class ProgressTaskbar {
public:
  ProgressTaskbar() {
    //
  }
  ~ProgressTaskbar() {
    if (taskbar != nullptr) {
      taskbar->Release();
    }
  }
  bool Initialzie() {
    hConsole = GetConsoleWindow();
    if (hConsole == nullptr) {
      // NO WINDOW
      return false;
    }
    auto hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
                               IID_ITaskbarList3, (void **)&taskbar);
    if (hr != S_OK) {
      //
    }
    return true;
  }
  bool State(TBPFLAG state) {
    if (taskbar != nullptr) {
      taskbar->SetProgressState(hConsole, TBPF_NOPROGRESS);
    }
    return true;
  }
  void Update(unsigned rate) {
    if (rate > 1000) {
      rate = 1000;
    }
    if (taskbar != nullptr) {
      taskbar->SetProgressValue(hConsole, rate, 1000);
    }
  }
  void Finish() {
    if (taskbar != nullptr) {
      taskbar->SetProgressState(hConsole, TBPF_NOPROGRESS);
    }
  }

private:
  ITaskbarList3 *taskbar{nullptr};
  HWND hConsole{nullptr};
};

Progressbar::Progressbar() {
  //
  taskbar = new ProgressTaskbar();
}

Progressbar::~Progressbar() {
  //
  delete taskbar;
}

bool Progressbar::Initialize() {
  static CoInitializeHelper conhelper;
  if (!taskbar->Initialzie()) {
    /// DEBUG ERROR
  }
  tick = std::chrono::system_clock::now();
  return true;
}

void Progressbar::Update(uint64_t bytes) {
  //
  // (completed+bytes) *1000/total
  auto now = std::chrono::system_clock::now();
  auto diff =
      std::chrono::duration_cast<std::chrono::microseconds>(now - tick).count();
  auto speed = bytes * 1000000 / diff; // bytes/S
  tick = now;
}

void Progressbar::Finish() {
  //
  taskbar->Finish();
}

void Progressbar::Refresh() {
  //
  tick = std::chrono::system_clock::now();
}