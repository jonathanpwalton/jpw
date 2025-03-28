#include "curl.hpp"
#include <curl/curl.h>

#include <iostream>

namespace curl {
  void print_size(size_t s) {
    if (s > size_t(1) << 40)
      printf("%4lu T", s >> 40);
    else if (s > 1 << 30)
      printf("%4lu G", s >> 30);
    else if (s > 1 << 20)
      printf("%4lu M", s >> 20);
    else if (s > 1 << 10)
      printf("%4lu K", s >> 10);
    else
      printf("%4lu B", s);
  }

  struct progress {
    FILE * file;
    std::string dst;
    curl_off_t size;
    size_t written;

    void print() {
      static char const * const BAR = "================================================================================================================";
      static char const * const SPC = "                                                                                                                ";
      static const int LEN = 75;
      
      printf("\x1b[1F");
      printf("\x1b[2K");
      
      int percent = (unsigned int) ((double) written / (double) size * 100.0);
      int barlenl = std::max((int) (std::min(percent / 50.0, 1.0) * LEN / 2.0), 0);
      int barlenr = std::max((int) ((percent - 50.0) / 50.0 * LEN / 2.0), 0);

      printf("[%.*s%.*s", barlenl, BAR, LEN / 2 - barlenl, SPC);
      if (barlenr > 0 && percent < 100) printf("=%2u%%", percent);
      else printf("%3u%%", percent);
      printf("%.*s%.*s]", barlenr, BAR, LEN / 2 - barlenr, SPC);
      printf("\n");
    }
  };

  static size_t write(char * src, size_t size, size_t n, progress * prog) {
    prog->written += size * n;
    prog->print();
    return fwrite(src, size, n, prog->file);
  }
}

bool curl::download(std::string const & url, std::string const & dst) {
  CURL * curl = curl_easy_init();
  FILE * file = fopen(dst.c_str(), "wb");

  if (!curl)
    return false;

  if (!file) {
    curl_easy_cleanup(curl);
    return false;
  }

  auto cleanup = [&] () {
    fclose(file);
    curl_easy_cleanup(curl);
  };
  
  progress progress = {file, dst.substr(dst.find_last_of("/") + 1), 0, 0};

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  
  {
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    
    if (curl_easy_perform(curl) != CURLE_OK) {
      cleanup();
      return false;
    }

    long response;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);

    if (response != 200) {
      cleanup();
      return false;
    }

    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &progress.size);

    if (progress.size < 0) {
      cleanup();
      return false;
    }
  }

  printf("\n");
  progress.print();

  {
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &progress.file);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl::write);

    if (curl_easy_perform(curl) != CURLE_OK) {
      cleanup();
      return false;
    }
  }

  cleanup();
  return true;
}