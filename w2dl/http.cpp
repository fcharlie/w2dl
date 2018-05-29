#include "stdafx.h"
//
#include "http.hpp"


bool HttpDownloader::Download(std::wstring_view url, std::wstring_view save,
                              const ProgressCallback &progress) {
  // one task
  return false;
}

bool HttpDownloader::PipeDownload(std::vector<std::wstring> &urls,
                                  const ProgressPipe &pp) {
  // pipe download task ... taskN
  return false;
}
