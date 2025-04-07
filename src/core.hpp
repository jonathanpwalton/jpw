#include <limits.h>
#include <unistd.h>

#include <curl/curl.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <string>
#include <exception>
#include <filesystem>
#include <format>
#include <algorithm>

#define INDENT get_indent()
#define argv get_argv()
#define root get_root()
#define repo get_repo()
#define cache get_cache()
#define store get_store()
#define mkdirs std::filesystem::create_directories

using error = std::runtime_error;
namespace std { using path = std::filesystem::path; };
using std::filesystem::is_directory;
using std::filesystem::is_regular_file;
using iterate = std::filesystem::directory_iterator;
using std::filesystem::exists;
using std::filesystem::begin;
using std::filesystem::end;

size_t & get_indent();
std::queue<std::string> & get_argv();
std::path const & get_root();
std::path const & get_repo();
std::path const & get_cache();
std::path const & get_store();

namespace cmd {
  void pull();
  void help();
};

inline void permit() {
  bool failed = false;

  try {
    if (!exists(root)) mkdirs(root);
    if (!exists(cache)) mkdirs(cache);
    if (!exists(repo)) mkdirs(repo);
    if (!exists(store)) mkdirs(store);
  } catch (std::filesystem::filesystem_error &) {
    failed = true;
  }

  if (failed || access(root.c_str(), W_OK) != 0)
    throw error("you are not permitted to perform this action");
}

inline std::string basename(std::string const & path) {
  return path.rfind('/') != std::string::npos ? path.substr(path.rfind('/') + 1) : path;
}

inline void begstage(std::string const & s) {
  std::cout << (INDENT == 0 ? ":: " : std::string(INDENT, ' ') + "=> " ) << s << '\n';
  INDENT += 3;
}

inline void endstage() {
  INDENT -= 3;
}

inline void warning(std::string const & s) {
  std::cout << std::string(INDENT, ' ') << "warning: " << s << "\n";
}

inline std::path download(std::string const & url, std::path const & dir, std::string const & label = "") {
  struct curlerror {};
  CURL * curl = curl_easy_init();

  using write_callback = size_t (*)(char *ptr, size_t size, size_t nmemb, std::ofstream * stream);
  using progress_callback = int (*)(std::string const * file, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

  if (!curl)
    throw error("failed to initalize curl");

  auto olabel = label;

  if (olabel.empty())
    olabel = basename(url);

  if (olabel.empty())
    olabel = url;

  std::string ofile = url;
  std::replace(ofile.begin(), ofile.end(), '/', '_');
  std::replace(ofile.begin(), ofile.end(), ':', '-');
  std::ofstream file(dir / ofile);

  try {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &olabel);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, (progress_callback) [] (std::string const * label, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) -> int {
      double percent = now <= total ? (double) now / (total != 0 ? total : UINT64_MAX) * 100.0 : 0.0;

      std::cout << "\033[A\r\033[0K" << 
      std::string(INDENT, ' ') << "downloading " << std::format("{:28.28}", *label) <<
      std::format(" {:3}% ", (short) percent) << "[" << std::format("{:32.32}", std::string((short) (percent / 100.0 * 32), '#')) << "] ";

      static auto const format = [] (curl_off_t n) {
        if (n < (1 << 10)) return std::format("  {:4}B", n);
        size_t pow = n < (1 << 20) ? 10 : n < (1 << 30) ? 20 : 30;
        auto suffix = pow == 10 ? "KiB" : pow == 20 ? "MiB" : "GiB";
        double f = n / (double) (1 << pow);
        return f >= 100 ? std::format("{:4.0f}{}", f, suffix) : f >= 10 ? std::format("{:2.1f}{}", f, suffix) : std::format("{:1.2f}{}", f, suffix);
      };

      if (total > 0) {
        std::cout << format(now) << " " << format(total);
      } else if (now > 0)
        std::cout << format(now);

      std::cout << std::endl;
      return 0;
    });

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (write_callback) [] (char * data, size_t, size_t size, std::ofstream * stream) -> size_t {
      auto start = stream->tellp();
      stream->write(data, size);
      return stream->tellp() - start;
    });

    std::cout << "downloading " << label << std::endl;
    if (curl_easy_perform(curl) != CURLE_OK)
      throw curlerror {};
  } catch (curlerror &) {
    curl_easy_cleanup(curl);
    throw error("failed to retrieve " + url);
  }

  curl_easy_cleanup(curl);

  return dir / ofile;
}

