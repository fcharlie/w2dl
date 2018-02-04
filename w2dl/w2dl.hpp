#ifndef W2DOWNLOAD_H
#define W2DOWNLOAD_H
#pragma once
#include "stdafx.h"
#include <string>
#include <list>
#include <Shlwapi.h>


struct dlitem {
	std::wstring url;
	std::wstring file;
};



class Downloader {
public:
	Downloader() = default;
	Downloader(const Downloader &) = delete;
	Downloader&operator=(const Downloader &) = delete;
	~Downloader() {
		///
	}
	size_t Append(const std::wstring &url) {
		dlist_.push_back(dlitem{ url,L"" });
		return dlist_.size();
	}
	size_t Append(const std::wstring &url, const std::wstring &file) {
		dlist_.push_back(dlitem{ url,file });
		return dlist_.size();
	}
	int Execute();
private:
	std::list<dlitem> dlist_;
};


#endif
