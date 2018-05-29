#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
#include <Shlwapi.h>
#include <WinInet.h>
#include <Windows.h>
#include <string>

#pragma comment(lib, "WinInet.lib")

class ErrorMessage {
public:
  ErrorMessage(DWORD errid) : lastError(errid), release_(false) {
    if (FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            nullptr, errid, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            (LPWSTR)&buf, 0, nullptr) == 0) {
      buf = const_cast<wchar_t *>(L"Unknown error");
    } else {
      release_ = true;
    }
  }
  ~ErrorMessage() {
    if (release_) {
      LocalFree(buf);
    }
  }
  const wchar_t *message() const { return buf; }
  DWORD LastError() const { return lastError; }

private:
  DWORD lastError;
  LPWSTR buf;
  bool release_;
  char reserved[sizeof(intptr_t) - sizeof(bool)];
};

namespace console {
namespace fc {
enum Color : WORD {
  Black = 0,
  White = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
  Blue = FOREGROUND_BLUE,
  Green = FOREGROUND_GREEN,
  Red = FOREGROUND_RED,
  Yellow = FOREGROUND_RED | FOREGROUND_BLUE,
  Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE,
  LightWhite = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |
               FOREGROUND_INTENSITY,
  LightBlue = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
  LightGreen = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
  LightRed = FOREGROUND_RED | FOREGROUND_INTENSITY,
  LightYellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
  LightMagenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
  LightCyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
};
}
namespace bc {
enum Color : WORD {
  Black = 0,
  White = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,
  Blue = BACKGROUND_BLUE,
  Green = BACKGROUND_GREEN,
  Red = BACKGROUND_RED,
  Yellow = BACKGROUND_RED | BACKGROUND_GREEN,
  Magenta = BACKGROUND_RED | BACKGROUND_BLUE,
  Cyan = BACKGROUND_GREEN | BACKGROUND_BLUE,
  LightWhite = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED |
               BACKGROUND_INTENSITY,
  LightBlue = BACKGROUND_BLUE | BACKGROUND_INTENSITY,
  LightGreen = BACKGROUND_GREEN | BACKGROUND_INTENSITY,
  LightRed = BACKGROUND_RED | BACKGROUND_INTENSITY,
  LightYellow = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
  LightMagenta = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
  LightCyan = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
};
}
} // namespace console

int BaseWriteConhostEx(HANDLE hConsole, WORD color, const wchar_t *buf,
                       size_t len) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  WORD oldColor = csbi.wAttributes;
  SetConsoleTextAttribute(hConsole, color);
  DWORD dwWrite;
  WriteConsoleW(hConsole, buf, len, &dwWrite, nullptr);
  SetConsoleTextAttribute(hConsole, oldColor);
  return dwWrite;
}
int BaseMessagePrintEx(WORD color, const wchar_t *format, ...) {
  HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
  wchar_t buf[16348];
  va_list ap;
  va_start(ap, format);
  auto l = _vswprintf_c_l(buf, 16348, format, nullptr, ap);
  va_end(ap);
  return BaseWriteConhostEx(hConsole, color, buf, l);
}

int BaseWriteConhost(HANDLE hConsole, WORD fcolor, const wchar_t *buf,
                     size_t len) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(hConsole, &csbi);
  WORD oldColor = csbi.wAttributes;
  WORD newColor = (oldColor & 0xF0) | fcolor;
  SetConsoleTextAttribute(hConsole, newColor);
  DWORD dwWrite;
  WriteConsoleW(hConsole, buf, len, &dwWrite, nullptr);
  SetConsoleTextAttribute(hConsole, oldColor);
  return dwWrite;
}

int BaseErrorMessagePrint(const wchar_t *format, ...) {
  HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
  wchar_t buf[16348];
  va_list ap;
  va_start(ap, format);
  auto l = _vswprintf_c_l(buf, 16348, format, nullptr, ap);
  va_end(ap);
  return BaseWriteConhost(hConsole, console::fc::LightRed, buf, l);
}

int BaseMessagePrint(WORD color, const wchar_t *format, ...) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  wchar_t buf[16348];
  va_list ap;
  va_start(ap, format);
  auto l = _vswprintf_c_l(buf, 16348, format, nullptr, ap);
  va_end(ap);
  return BaseWriteConhost(hConsole, color, buf, l);
}

class WinINetObject {
public:
  WinINetObject(HINTERNET hInternet) : hInternet_(hInternet) {}
  operator HINTERNET() { return hInternet_; }
  operator bool() { return hInternet_ != nullptr; }
  WinINetObject() {
    if (hInternet_) {
      InternetCloseHandle(hInternet_);
    }
  }

private:
  HINTERNET hInternet_;
};

struct WinINetRequestURL {
  int nPort = 0;
  int nScheme = 0;
  std::wstring scheme;
  std::wstring host;
  std::wstring path;
  std::wstring username;
  std::wstring password;
  std::wstring extrainfo;
  bool Parse(const std::wstring &url) {
    URL_COMPONENTS urlComp;
    DWORD dwUrlLen = 0;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;

    if (!InternetCrackUrlW(url.data(), dwUrlLen, 0, &urlComp)) {
      return false;
    }
    scheme.assign(urlComp.lpszScheme, urlComp.dwSchemeLength);
    host.assign(urlComp.lpszHostName, urlComp.dwHostNameLength);
    path.assign(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    nPort = urlComp.nPort;
    nScheme = urlComp.nScheme;
    if (urlComp.lpszUserName) {
      username.assign(urlComp.lpszUserName, urlComp.dwUserNameLength);
    }
    if (urlComp.lpszPassword) {
      password.assign(urlComp.lpszPassword, urlComp.dwPasswordLength);
    }
    if (urlComp.lpszExtraInfo) {
      extrainfo.assign(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
    }
    return true;
  }
};

bool wurlstat(const std::wstring &url) {
  WinINetRequestURL zurl;
  if (!zurl.Parse(url)) {
    BaseErrorMessagePrint(L"Wrong URL: %s\n", url.c_str());
    return false;
  }
  DeleteUrlCacheEntryW(url.c_str());
  WinINetObject hInet = InternetOpenW(
      L"wrulstat/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (!hInet) {
    ErrorMessage err(GetLastError());
    BaseErrorMessagePrint(L"InternetOpenW(): %s", err.message());
    return false;
  }

  DWORD dwOption = HTTP_PROTOCOL_FLAG_HTTP2;
  if (!InternetSetOptionW(hInet, INTERNET_OPTION_ENABLE_HTTP_PROTOCOL,
                          &dwOption, sizeof(dwOption))) {
    ErrorMessage err(GetLastError());
    BaseErrorMessagePrint(L"InternetConnectW(): %s", err.message());
  }

  WinINetObject hConnect =
      InternetConnectW(hInet, zurl.host.c_str(), (INTERNET_PORT)zurl.nPort,
                       nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, NULL);
  if (!hConnect) {
    ErrorMessage err(GetLastError());
    BaseErrorMessagePrint(L"InternetConnectW(): %s", err.message());
    return false;
  }
  DWORD dwOpenRequestFlags =
      INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | INTERNET_FLAG_KEEP_CONNECTION |
      INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI |
      INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
      INTERNET_FLAG_RELOAD;

  DWORD64 dwContentLength = 1;
  WinINetObject hRequest =
      InternetOpenUrlW(hInet, url.c_str(), nullptr, 0, dwOpenRequestFlags, 0);
  if (zurl.nScheme == INTERNET_SCHEME_HTTP ||
      zurl.nScheme == INTERNET_SCHEME_HTTPS) {
    DWORD dwbuflen = 0;
    HttpQueryInfoW(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, nullptr, &dwbuflen,
                   nullptr);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      return false;
    }
    std::string buf;
    buf.resize(dwbuflen, '\0');
    HttpQueryInfoW(hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, &buf[0], &dwbuflen,
                   nullptr);
    printf("%zu\n%ls\n", buf.size(), (const wchar_t *)buf.data());
	bool ish2 = false;
	DWORD dwlen = sizeof(dwOption);
	if (InternetQueryOptionW(hRequest, INTERNET_OPTION_HTTP_PROTOCOL_USED, &dwOption, &dwlen)) {
		if ((dwOption &HTTP_PROTOCOL_FLAG_HTTP2)!=0) {
			printf("Request: HTTP/2.0\n");
		}
	}
  }

  return true;
}