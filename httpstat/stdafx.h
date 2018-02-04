// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <winhttp.h>

#define HTTPSTAT_USERAGENT  L"httpstat/1.0"
#ifdef _WIN64
#define MOZ_USERAGENT L"Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
#else
#define MOZ_USERAGENT L"Mozilla/5.0 (Windows NT 10.0; Win32; x32)"
#endif

#include <chrono>


// TODO: 在此处引用程序需要的其他头文件
