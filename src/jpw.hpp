#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <fstream>
#include <format>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define argv get_args()
std::vector<std::string> const & get_args();
bool download(std::string const & url);

#define prefix get_prefix()
std::string const & get_prefix();

#define cache std::string(prefix + "/cache")
#define packages std::string(prefix + "/packages")

inline std::string basename(std::string const & path) {
  return path.rfind('/') != path.npos ? path.substr(path.rfind('/') + 1) : path;
}

inline void error(std::string const & msg, bool predicate = true) {
  if (!predicate) return;
  std::cerr << "error: " << msg << "\n";
  exit(1);
}

inline bool mkdir(std::string const & dir) {
  return mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
}

inline bool chdir(std::string const & dir) {
  return chdir(dir.c_str()) == 0;
}

inline bool exists(std::string const & path) {
  return access(path.c_str(), F_OK) == 0;
}

inline bool writeable(std::string const & path) {
  return access(path.c_str(), W_OK) == 0;
}

inline bool permitted() {
  return !exists(prefix) ? mkdir(prefix) : writeable(prefix);
}

inline void help() {
  using std::endl;

  std::cout <<
    "usage: jpw <command>\n\n" <<
    "commands:\n" <<
    "  update [available | installed]                                           update available, installed, or all packages\n" <<
    "  install {<package> [-p <provider>] [-v <version>] [-c <config>] ...}     install one or more packages\n" <<
    "  remove {<package> ...}                                                   uninstall and remove one or more packages\n" <<
    "  list {available | installed}                                             list all available or installed packages\n" <<
    "  help                                                                     display this help text and exit\n";
}

namespace update {
  void available();
  void installed();

  inline void all() {
    available();
    installed();
  }
}

namespace list {
  void available();
  void installed();
}

void install();
