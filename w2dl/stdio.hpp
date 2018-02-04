#ifndef STDIO_HPP
#define STDIO_HPP
#pragma once

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <string>
#include <string_view>

template <typename T> T Argument(T value) noexcept { return value; }
template <typename T>
T const *Argument(std::basic_string<T> const &value) noexcept {
	return value.c_str();
}
template <typename... Args>
int StringPrint(wchar_t *const buffer, size_t const bufferCount,
	wchar_t const *const format, Args const &... args) noexcept {
	int const result = swprintf(buffer, bufferCount, format, Argument(args)...);
	// ASSERT(-1 != result);
	return result;
}
// https://docs.microsoft.com/en-us/windows/console/console-screen-buffer-info-str
class Progressbar {
private:
	size_t WindowSize() {
		auto hStderr = GetStdHandle(STD_ERROR_HANDLE);
		if (GetFileType(hStderr) != FILE_TYPE_CHAR) {
			return 100;
		}
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(hStderr,&info);
		return info.dwSize.X;
	}
public:
	Progressbar() = default;
	Progressbar(const Progressbar &) = delete;
	Progressbar&operator=(const Progressbar&) = delete;
	void WriteTitle(std::wstring_view title) {

	}
	void Update(uint64_t data) {
		if (total == 0) {

		}
	}
	void Refresh() {

	}
private:
	std::wstring data;
	uint64_t total{ 0 };
};

#endif
