// w2dl.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

size_t WindowSize() {
	auto hStderr = GetStdHandle(STD_ERROR_HANDLE);
	if (GetFileType(hStderr) != FILE_TYPE_CHAR) {
		return 100;
	}
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(hStderr, &info);
	return info.dwSize.X;
}

int wmain(int argc,wchar_t **argv)
{
	auto sz = WindowSize();
	printf("Window Size: %zu\n", sz);
    return 0;
}

