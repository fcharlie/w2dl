// w2dl.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "stdio.hpp"
#include "w2dl.hpp"
Outdevice out;

size_t WindowSize() {
  auto hStderr = GetStdHandle(STD_ERROR_HANDLE);
  if (GetFileType(hStderr) != FILE_TYPE_CHAR) {
    return 100;
  }
  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(hStderr, &info);
  return info.dwSize.X;
}

std::wstring ContentDispositionFilename(const wchar_t *buf, DWORD dwlen);

int wmain(int argc, wchar_t **argv) {
  if (!out.Initialize()) {
    fprintf(stderr, "Initialize out device error\n");
    return -1;
  }
  const wchar_t *text[] = {L"attachment; filename=\"hello world\"",
                           L"attachment; filename=1234556.txt"};
  for (auto t : text) {
    auto w = ContentDispositionFilename(t, wcslen(t));
    if (!w.empty()) {
      wprintf(L"%s --> %s\n", t, w.data());
    }
  }
  auto sz = WindowSize();
  printf("Window Size: %zu\n", sz);
  return 0;
}
