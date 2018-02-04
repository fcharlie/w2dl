#include "stdafx.h"
#include "w2dl.hpp"
#include <winhttp.h>
#pragma comment(lib,"winhttp")
#pragma comment(lib,"Shlwapi")

#define MinWarp(a,b) ((b<a)?b:a)

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
	NetObject hNet = WinHttpOpen(W2DL_USERAGENT,
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hNet) {
		return false;
	}
	DWORD dwOption = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
	if (!WinHttpSetOption(hNet,
		WINHTTP_OPTION_REDIRECT_POLICY,
		&dwOption, sizeof(DWORD))) {
		return false;
	}
	// HTTP2 support
	dwOption = WINHTTP_PROTOCOL_FLAG_HTTP2;
	if (!WinHttpSetOption(hNet,
		WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
		&dwOption, sizeof(dwOption))) {
		fprintf(stderr, "HTTP2 is not supported\n");
	}
	NetObject hConnect = WinHttpConnect(hNet, ui.szHostName,
		(INTERNET_PORT)ui.Port, 0);
	if (!hConnect) {
		return false;
	}
	DWORD flags = WINHTTP_FLAG_ESCAPE_DISABLE;/// Because URI_INFO escape done.
	if (ui.nScheme == INTERNET_SCHEME_HTTPS) {
		flags |= WINHTTP_FLAG_SECURE;
	}
	NetObject hRequest = WinHttpOpenRequest(
		hConnect, L"GET", ui.PathQuery.data(), nullptr, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (!hRequest) {
		return false;
	}
	if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
		return false;
	}
	if (WinHttpReceiveResponse(hRequest, NULL) == FALSE) {
		return false;
	}
	DWORD dwStatusCode = 0;
	DWORD dwXsize = sizeof(dwStatusCode);
	if (!WinHttpQueryHeaders(hRequest,
		WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwXsize,
		WINHTTP_NO_HEADER_INDEX)) {
		return false;
	}
	/// HTTP Statuscode download need body, so must 200,201
	/// 206 part download
	if (dwStatusCode != 200 && dwStatusCode != 201 &&dwStatusCode !=206) {
		/// print error
		return false;
	}
	uint64_t dwContentLength = 0;
	wchar_t buffer[512];
	dwXsize = 64;
	/// chunked encoding not Content-Length
	if (WinHttpQueryHeaders(hRequest,
		WINHTTP_QUERY_CONTENT_LENGTH,
		WINHTTP_HEADER_NAME_BY_INDEX, buffer, &dwXsize,
		WINHTTP_NO_HEADER_INDEX)) {
		wchar_t *cx = nullptr;
		dwContentLength = wcstoull(buffer, &cx, 10);
	}
	///////////////////////////// create file
	std::wstring df;
	if (file.empty()) {
		// See https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Headers/Content-Disposition
		// Content-Disposition
		dwXsize = 512;
		const constexpr int dipositionlen = sizeof("attachment; filename=\"\"") - 1;
		if (WinHttpQueryHeaders(hRequest, 
			WINHTTP_QUERY_CUSTOM,
			L"Content-Disposition", buffer, &dwXsize,
			WINHTTP_NO_HEADER_INDEX) &&dwXsize>dipositionlen) {
			df.assign(buffer + dipositionlen - 1, dwXsize - dipositionlen);
		}
		else {
			df = PathFindFileNameW(ui.szUrlPath);
		}
	}
	else {
		df = file;
	}
	//// createfile;
	DWORD dwSize = 0;
	uint64_t totalsize = 0;
	char recvbuf[16384];
	do{
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			break;
		}
		totalsize += dwSize;
		auto dwSizeN = dwSize;
		while (dwSizeN > 0) {
			DWORD dwDownloaded = 0;
			if (!WinHttpReadData(hRequest,
				(LPVOID)recvbuf,
				MinWarp(sizeof(recvbuf), dwSizeN),
				&dwDownloaded)) {
				break;
			}
			//fwrite(recvbuf, 1, dwDownloaded, stderr);
			dwSizeN -= dwDownloaded;
		}
	} while (dwSize > 0);
	return true;
}


int Downloader::Execute() {
	return 0;
}