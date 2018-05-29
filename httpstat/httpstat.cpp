//

#include "stdafx.h"
#include <string>
#include <string_view>
#include <unordered_map>
#pragma comment(lib,"winhttp")

#include "stdio.hpp"

#define MinWarp(a,b) ((b<a)?b:a)

Outdevice out;

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
		if (WinHttpCrackUrl(url.data(), static_cast<DWORD>(url.size()), ICU_ESCAPE, &uc)!=TRUE) {
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

bool HeaderStat(bool ishttp2, const wchar_t *data, size_t len,int statuscode)
{
	wprintf(L"%s\n", data);
	auto end = data + len;
	auto p = wcsstr(data, L"\r\n");
	if (p == nullptr) {
		return false;
	}
	int color = (statuscode >= 200 && statuscode <= 299) ? 33 : 31;
	out.Print(L"HTTP/\x1b[1;35m%s\x1b[0m \x1b[%dm%d %s\x1b[0m\n",
		(ishttp2 ? L"2.0" : std::wstring(data+5,3).c_str()), 
		color,
		statuscode, std::wstring(data + 13, p - data - 13));

	auto pre = p + 2;
	while (p<end) {
		p = wcsstr(pre, L"\r\n");
		if (p == nullptr) {
			break;
		}
		auto x = wmemchr(pre, L':', p - pre);
		if (x == nullptr) {
			out.Print(L"%s\n", std::wstring(pre, p - pre));
			break;
		}
		out.Print(L"%s: \x1b[33m%s\x1b[0m\n", std::wstring(pre, x - pre), std::wstring(x + 2, p - x - 2));
		pre = p + 2;
	}
	return true;
}

// https://blogs.windows.com/buildingapps/2015/11/23/demystifying-httpclient-apis-in-the-universal-windows-platform/
// Windows 10 Windows.Web.Http support HTTP2
bool HttpStat(std::wstring_view url)
{
	URI_INFO ui;
	if (!ui.CrackURI(url)) {
		return false;
	}
	NetObject hNet = WinHttpOpen(HTTPSTAT_USERAGENT, 
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
	auto startconnect= std::chrono::system_clock::now();
	NetObject hConnect = WinHttpConnect(hNet,ui.szHostName,
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
	/// NEED Known is HTTP2
	bool ish2 = false;
	DWORD dwlen = sizeof(dwOption);
	if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_HTTP_PROTOCOL_USED, &dwOption, &dwlen)) {
		if ((dwOption &WINHTTP_PROTOCOL_FLAG_HTTP2)!=0) {
			ish2 = true;
		}
	}

	BOOL bResult = FALSE;
	bResult = ::WinHttpQueryHeaders(hRequest, 
		WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, 
		NULL, 
		&dwlen,
		WINHTTP_NO_HEADER_INDEX);
	int headerlen = dwlen;
	std::vector<wchar_t> hbuf(dwlen + 1);
	auto whd = hbuf.data();
	size_t contentLength = 0;
	::WinHttpQueryHeaders(hRequest, 
		WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, 
		whd, 
		&dwlen,
		WINHTTP_NO_HEADER_INDEX);
	DWORD dwStatusCode = 0;
	DWORD dwXsize = sizeof(dwStatusCode);
	if (!WinHttpQueryHeaders(hRequest,
		WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwXsize,
		WINHTTP_NO_HEADER_INDEX)) {
		return false;
	}
	if (dwStatusCode < 200 || dwStatusCode > 299) {
		return HeaderStat(ish2,whd, headerlen,dwStatusCode);
	}
	char recvbuf[8192];
	DWORD dwSize = 0;
	uint64_t totalsize = 0;
	do {
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
			dwSizeN -= dwDownloaded;
		}
	} while (dwSize > 0);

	auto connectend = std::chrono::system_clock::now();
	HeaderStat(ish2, whd, headerlen, dwStatusCode);
	out.Print(L"total:\t\x1b[32m%lld ms\x1b[0m\n", 
		std::chrono::duration_cast<std::chrono::milliseconds>(connectend - startconnect).count());
	return true;
}

// httpstat
int wmain(int argc,wchar_t **argv)
{
	if (!out.Initialize()) {
		fprintf(stderr, "Initialize out device error\n");
		return -1;
	}
	if (argc < 2) {
		out.Error(L"\x1b[31musage: %s url\x1b[0m\n", argv[0]);
		return 1;
	}
	if (HttpStat(argv[1])) {
		return 0;
	}
    return 1;
}

