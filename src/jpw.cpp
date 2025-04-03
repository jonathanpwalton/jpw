#include <curl/curl.h>

#include "jpw.hpp"

static std::vector<std::string> _args;

int main(int _argc, char ** _argv) {
  for (int i = 0; i < _argc; i++) _args.push_back(_argv[i]);

  if (argv.size() == 1) {
    error("a command is required, try '" + argv[0] + " help' for more information");
  } else if (argv[1] == "help") {
    error("the 'help' command does not take any arguments", argv.size() != 2);
    help();
  } else if (argv[1] == "install") {
    error("the 'install' command must be followed by one or more package arguments in the form '<package> [-p <provider>] [-v <version>] [-c <config>]'", argv.size() == 2);
    install();
  } else if (argv[1] == "list") {
    if (argv.size() == 3 && argv[2] == "available")
      list::available();
    else if (argv.size() == 3 && argv[2] == "installed")
      list::installed();
    else
      error("the 'list' command requires one argument of {available | installed}");
  } else if (argv[1] == "update") {
    if (argv.size() == 2)
      update::all();
    else if (argv.size() == 3 && argv[2] == "available")
      update::available();
    else if (argv.size() == 3 && argv[2] == "installed")
      update::installed();
    else
      error("the 'update' command requires zero or one argument of [available | installed]");
  } else
    error("the command '" + argv[1] + "' is not recognized, try '" + argv[0] + " help' for more information");

  return 0;
}

std::vector<std::string> const & get_args() {
  return _args;
}

bool download(std::string const & url) {
  struct Data { std::ofstream stream = {}; size_t length = 0; };
  using HeaderFunction = size_t(*) (char * buffer, size_t size, size_t nitems, Data * data);
  using WriteFunction = size_t(*) (char * ptr, size_t size, size_t nmemb, Data * data);

  CURL * curl = curl_easy_init();
  if (!curl) return false;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

  Data data;
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderFunction([](char * buffer, size_t size, size_t nitems, Data * data) -> size_t {
    std::string str(buffer, buffer + nitems);
    std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::tolower(c); });
    if (str.starts_with("content-length:"))
      data->length = std::stoull(str.substr(15));
    return nitems * size;
    }));

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction([](char * ptr, size_t size, size_t nmemb, Data * data) -> size_t {
    auto pos = data->stream.tellp();
    data->stream.write(ptr, size * nmemb);

    if (data->length != 0) {
      float percent = (float) data->stream.tellp() / (float) data->length;
      size_t barlen = percent * 78.0F;

      std::cout << "[" << std::string(barlen, '=') << std::string(78 - barlen, ' ') << "]\r" << std::flush;

      if (barlen == 78)
        std::cout << std::endl;
    }
    return data->stream.tellp() - pos;
    }));

  data.stream = std::ofstream(basename(url), std::ios::binary);

  if (!data.stream.good()) {
    data.stream.close();
    unlink(basename(url).c_str());
    curl_easy_cleanup(curl);
    return false;
  }

  std::cout << std::format("{:^80}", "downloading " + basename(url)) << std::endl;

  if (curl_easy_perform(curl) != CURLE_OK) {
    data.stream.close();
    unlink(basename(url).c_str());
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_cleanup(curl);
  return true;
}

std::string const & get_prefix() {
  static std::string const p = "/var/lib/jpw";
  return p;
}
