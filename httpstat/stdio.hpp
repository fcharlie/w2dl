﻿#ifndef HTTPSTAT_STDIO_HPP
#define HTTPSTAT_STDIO_HPP
#pragma once

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <string>
#include <utility>
#include <string_view>

template <typename T>
T const *Argument(std::basic_string<T> const &value) noexcept {
	return value.data();
}

template <typename T> T Argument(T value) noexcept { return value; }

template <typename... Args>
int StringPrint(wchar_t *const buffer, size_t const bufferCount,
	wchar_t const *const format, Args const &... args) noexcept {
	int const result = swprintf(buffer, bufferCount, format, Argument(args)...);
	// ASSERT(-1 != result);
	return result;
}

class Outdevice {
private:
	int WriteFileImpl(HANDLE hFile,const wchar_t *text, size_t len) {
		std::string str;
		auto N = WideCharToMultiByte(CP_UTF8, 0, text, (int)len, nullptr, 0, nullptr,
			nullptr);
		str.resize(N);
		WideCharToMultiByte(CP_UTF8, 0, text, (int)len, &str[0], N, nullptr, nullptr);
		DWORD dwrite = 0;
		if (!WriteFile(hStderr, str.data(), N, &dwrite, nullptr)) {
			return -1;
		}
		return dwrite;
	}
public:
	Outdevice() = default;
	Outdevice(const Outdevice &) = delete;
	Outdevice&operator=(const Outdevice &) = delete;
	bool Initialize() {
		hStderr = GetStdHandle(STD_ERROR_HANDLE);
		if (hStderr == INVALID_HANDLE_VALUE) {
			return false;
		}
		if (GetFileType(hStderr) == FILE_TYPE_CHAR) {
			erristty = true;
			DWORD dwMode = 0;
			if (!GetConsoleMode(hStderr, &dwMode)) {
				return false;
			}

			dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			if (!SetConsoleMode(hStderr, dwMode)) {
				return false;
			}
		}
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdout == INVALID_HANDLE_VALUE) {
			return false;
		}
		if (GetFileType(hStdout) == FILE_TYPE_CHAR) {
			outistty = true;
			DWORD dwMode = 0;
			if (!GetConsoleMode(hStdout, &dwMode)) {
				return false;
			}
			dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			if (!SetConsoleMode(hStdout, dwMode)) {
				return false;
			}
		}
		return true;
	}
	template<typename ...Args>
	int Print(const wchar_t *format, Args... args) {
		std::wstring buffer;
		size_t size = StringPrint(nullptr, 0, format, args...);
		buffer.resize(size);
		size = StringPrint(&buffer[0], buffer.size() + 1, format, std::forward<Args>(args)...);
		if (outistty) {
			DWORD dwrite = 0;
			if (!WriteConsoleW(hStdout, buffer.data(), size, &dwrite, nullptr)) {
				return -1;
			}
			return dwrite;
		}
		return WriteFileImpl(hStdout, buffer.data(), size);
	}
	template<typename ...Args>
	int Error(const wchar_t *format, Args... args) {
		std::wstring buffer;
		size_t size = StringPrint(nullptr, 0, format, args...);
		buffer.resize(size);
		size = StringPrint(&buffer[0], buffer.size() + 1, format, args...);
		if (outistty) {
			DWORD dwrite = 0;
			if (!WriteConsoleW(hStderr, buffer.data(), size, &dwrite, nullptr)) {
				return -1;
			}
			return dwrite;
		}
		return WriteFileImpl(hStderr, buffer.data(), size);
	}
private:
	HANDLE hStderr{ INVALID_HANDLE_VALUE };
	HANDLE hStdout{ INVALID_HANDLE_VALUE };
	bool erristty{ false };
	bool outistty{ false };
};

#endif
