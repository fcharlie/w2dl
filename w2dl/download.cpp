#include "stdafx.h"
#include "w2dl.hpp"
#include <winhttp.h>
#pragma comment(lib,"winhttp")
#pragma comment(lib,"Shlwapi")

#define W2DL_USERAGENT  L"w2dl/1.0"
#ifdef _WIN64
#define MOZ_USERAGENT L"Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
#else
#define MOZ_USERAGENT L"Mozilla/5.0 (Windows NT 10.0; Win32; x32)"
#endif

#ifndef WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL
#define WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL 133
#endif

#ifndef WINHTTP_PROTOCOL_FLAG_HTTP2
#define WINHTTP_PROTOCOL_FLAG_HTTP2 0x1
#endif

// https://www.ietf.org/rfc/rfc1738.txt
struct URI_INFO {
	int Port{ 0 };
	WCHAR szScheme[32] = { 0 };
	WCHAR szHostName[256] = { 0 }; //https://tools.ietf.org/html/rfc1123#section-2.1
	WCHAR szUserName[128] = { 0 };
	WCHAR szPassword[128] = { 0 };
	WCHAR szUrlPath[4096] = { 0 };
	WCHAR szExtraInfo[1024] = { 0 };
	std::wstring PathQuery;
	int nScheme{ 0 };
	bool CrackURI(std::wstring_view url) {
		URL_COMPONENTS uc;
		ZeroMemory(&uc, sizeof(uc));
		uc.dwStructSize = sizeof(uc);
		//
		uc.lpszScheme = szScheme;
		uc.dwSchemeLength = sizeof(szScheme);
		//
		uc.lpszHostName = szHostName;
		uc.dwHostNameLength = sizeof(szHostName);
		//
		uc.lpszUserName = szUserName;
		uc.dwUserNameLength = sizeof(szUserName);

		uc.lpszPassword = szPassword;
		uc.dwPasswordLength = sizeof(szPassword);

		uc.lpszUrlPath = szUrlPath;
		uc.dwUrlPathLength = sizeof(szUrlPath);

		uc.lpszExtraInfo = szExtraInfo;
		uc.dwExtraInfoLength = sizeof(szExtraInfo);
		if (WinHttpCrackUrl(url.data(), static_cast<DWORD>(url.size()), ICU_ESCAPE, &uc) != TRUE) {
			return false;
		}
		nScheme = uc.nScheme;
		Port = uc.nPort;
		PathQuery.assign(szUrlPath);
		if (uc.dwExtraInfoLength > 0) {
			PathQuery.append(szExtraInfo);
		}
		return true;
	}
};
struct NetObject {
	NetObject(const NetObject &) = delete;
	NetObject&operator=(const NetObject &) = delete;
	NetObject(HINTERNET hNet_) :hNet(hNet_) {}
	operator HINTERNET() {
		return hNet;
	}
	operator bool() {
		return hNet != nullptr;
	}
	NetObject() {
		if (hNet) {
			WinHttpCloseHandle(hNet);
		}
	}
	HINTERNET hNet;
};

bool HttpGet(const std::wstring &url,const std::wstring &file)
{
	URI_INFO ui;
	if (!ui.CrackURI(url)) {
		return false;
	}

	///////////////////////////// create file
	std::wstring df;
	if (file.empty()) {
		// Content-Disposition

		auto filename = PathFindFileNameW(ui.szUrlPath);
	}
	//PathFindFileName
	return true;
}


int Downloader::Execute() {
	return 0;
}