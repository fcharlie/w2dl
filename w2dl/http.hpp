#ifndef HTTPENGINE_HPP
#define HTTPENGINE_HPP
#pragma once
#include <string>
#include <string_view>
#include <functional>

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa383138%28v=vs.85%29.aspx

class HttpEngine {
public:
  HttpEngine() = default;
  HttpEngine(const HttpEngine &) = delete;
  HttpEngine &operator=(const HttpEngine &) = delete;

private:
};

using ProgressCallback = std::function<bool(uint64_t, uint64_t)>;
using ProgressPipe = std::function<bool(int, uint64_t, uint64_t)>;
class HttpDownloader {
public:
  HttpDownloader() = default;
  HttpDownloader(const HttpDownloader &) = delete;
  HttpDownloader &operator=(const HttpDownloader &) = delete;
  bool Download(std::wstring_view url,std::wstring_view save,const ProgressCallback &progress);
  bool PipeDownload(std::vector<std::wstring> &urls,const ProgressPipe &pp);
private:
};

#endif
