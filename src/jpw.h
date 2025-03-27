#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>

static char const * const DIR = "/var/lib/jpw";
static char const * const DBA = "/var/lib/jpw/available";
static char const * const DBI = "/var/lib/jpw/installed";

static int const ERR_CURL_INIT=1;
static int const ERR_FILE_OPEN=2;
static int const ERR_CURL_NOOK=3;
static int const ERR_FILE_SIZE=4;
static int const ERR_HTTP_N200=5;

static inline char const * gnu_basename (char const * const path) {
  char *base = strrchr(path, '/');
  return base ? base+1 : path;
}

static inline void ferasef(FILE * stream) {
  fprintf(stream, "\x1b[1F");
  fprintf(stream, "\x1b[2K");
}

static inline void setupdirs(char ** argv) {
  if (mkdir("/var", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
    fprintf(stderr, "error: command '%s' requires root privilege\n", argv[1]);
    exit(1);
  }

  if (mkdir("/var/lib", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
    fprintf(stderr, "error: command '%s' requires root privilege\n", argv[1]);
    exit(1);
  }

  if (mkdir("/var/lib/jpw", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
    fprintf(stderr, "error: command '%s' requires root privilege\n", argv[1]);
    exit(1);
  }
}

static inline int download_size(const char *url, long *size) {
  CURL *curl = curl_easy_init();
  if (!curl) return ERR_CURL_INIT;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  CURLcode result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    curl_easy_cleanup(curl);
    return ERR_CURL_NOOK;
  }

  long response_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  if (response_code != 200) {
    curl_easy_cleanup(curl);
    return ERR_HTTP_N200;
  }

  result = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, size);

  if (result != CURLE_OK || *size < 0) {
    curl_easy_cleanup(curl);
    return ERR_CURL_NOOK;
  }

  curl_easy_cleanup(curl);
  return 0;
}

struct download_info {
  FILE * file;
  char const * const dst;
  curl_off_t size;
  size_t written;
};

static inline void download_progress(struct download_info * info) {
  char const * const BAR = "========================================================";
  char const * const SPC = "                                                        ";

  unsigned int percent = (unsigned int) ((double) info->written / (double) info->size * 100.0);
  unsigned int bar = (int) ((double) percent / 100.0 * 56.0);
  ferasef(stdout);
  fprintf(stdout, "%.16s%.*s", info->dst, (int) fmax(16.0 - strlen(info->dst), 0.0), SPC);
  fprintf(stdout, " %3u%% ", percent);
  fprintf(stdout, "[%.*s%.*s]\n", bar, BAR, 56 - bar, SPC);
}

static inline size_t download_write(char * src, size_t size, size_t nmemb, struct download_info * info) {
  info->written += size * nmemb;
  download_progress(info);
  return fwrite(src, size, nmemb, info->file);
}

static inline int download(char const * const dst, char const * const url) {
  CURL * curl = curl_easy_init();
  if (!curl) return ERR_CURL_INIT;

  FILE * file = fopen(dst, "wb");
  if (!file) return ERR_FILE_OPEN;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

  CURLcode result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    curl_easy_cleanup(curl);
    return ERR_CURL_NOOK;
  }

  long response_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  if (response_code != 200) {
    curl_easy_cleanup(curl);
    return ERR_HTTP_N200;
  }

  curl_off_t size;
  result = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &size);

  if (result != CURLE_OK || size < 0) {
    curl_easy_cleanup(curl);
    return ERR_CURL_NOOK;
  }

  struct download_info info = {
    .file = file,
    .dst = gnu_basename(dst),
    .size = size,
    .written = 0
  };

  curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &info);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_write);

  fprintf(stdout, "\n");
  download_progress(&info);
  result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    fclose(file);
    curl_easy_cleanup(curl);
    return ERR_CURL_NOOK;
  }

  fclose(file);
  curl_easy_cleanup(curl);
  return 0;
}
