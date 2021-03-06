// wurlstat.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
bool wurlstat(const std::wstring &url);
/// TO test wininet HTTP2
int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %ls url\n", argv[0]);
    return 1;
  }
  wurlstat(argv[1]);
  return 0;
}
