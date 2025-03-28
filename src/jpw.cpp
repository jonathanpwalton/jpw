#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>
#include "json.hpp"
#include "curl.hpp"

using json = nlohmann::json;

struct target {
  std::string pkg;
  std::string ver;
  json *verinfo;
  std::string cfg;
  bool upd;
};

char const * const NAME = "jpw";
char const * const VERSION = "2025.03.28";

std::vector<std::string> const DIRS = {"/var", "/var/lib", "/var/lib/jpw", "/var/lib/jpw/pkg"};
char const * const DBI = "/var/lib/jpw/installed.json";
char const * const DBA = "/var/lib/jpw/available.json";

void mkdirs();
bool shcall(std::string const & command);
void header(std::string const & str);
void require_root();
void update_available();
void update_installed();
void install(std::vector<target> const & targets);

int main(int argc, char ** argv) {
  std::vector<std::string> args;
  for (int i = 0; i < argc; i++) args.push_back(argv[i]);
  
  if (args.size() < 2) {
    std::cerr << "error: a command is required, see '" << args[0] << " help' for more information\n";
    return 1;
  } else if (args[1] == "help") {
    std::cout <<
    NAME << " " << VERSION << ", a simplicty-focused package manager for GNU/Linux\n\n" <<
    "usage: " << args[0] << " <command>\n\n" <<
    "commands:\n" <<
    "  update [available | installed]                         update available, installed, or all packages\n" <<
    "  install {<package> [-v <version>] [-c <config>] ...}   install one or more packages\n" <<
    "  list {available | installed}                           list available or installed packages\n" <<
    "  help                                                   display this help text and quit\n";
  } else if (args[1] == "update") {
    require_root();
    mkdirs();

    if (args.size() == 2) {
      update_available();
      update_installed();
    } else if (args.size() == 3 && args[2] == "available") {
      update_available();
    } else if (args.size() == 3 && args[2] == "installed") {
      update_installed();
    } else {
      std::cerr << "error: command 'update' takes zero or exactly one argument of {available | installed}\n";
      return 1;
    }
  } else if (args[1] == "install") {
    require_root();
    mkdirs();

    if (access(DBA, F_OK) != 0) {
      std::cerr << "error: the available package database has not been initialized, run '" << args[0] << " update available' first\n";
      return 1;
    } else if (args.size() == 2) {
      std::cerr << "error: command 'install' requires one or more packages optionally with version (-v VERSION) and/or configuration (-c CONFIG)\n";
      return 1;
    }

    json available = json::parse(std::ifstream(DBA));
    std::vector<target> targets;

    for (size_t i = 2; i < args.size(); i++) {
      std::string pkg = args[i];
      std::string ver = "latest";
      std::string cfg = "";
      bool upd = true;

      if (i + 1 < args.size() && args[i + 1] == "-v") {
        i++;

        if (i + 1 >= args.size()) {
          std::cerr << "error: package '" << pkg << "' has the version flag enabled, but no version was provided\n";
          return 1;
        }
        
        ver = args[++i];
        upd = ver == "latest";
      }

      if (i + 1 < args.size() && args[i + 1] == "-c") {
        i++;

        if (i + 1 >= args.size()) {
          std::cerr << "error: package '" << pkg << "' has the config flag enabled, but no config was provided\n";
          return 1;
        }
        
        cfg = args[++i];
      }

      if (!available.contains(pkg)) {
        std::cerr << "warning: package '" << pkg << "' not found in the available database, skipping\n";
        continue;
      }

      if (ver == "latest") {
        ver = available.at(pkg).items().begin().key();
      } else if (!available.at(pkg).contains(ver)) {
        std::cerr << "warning: package '" << pkg << "' has no version '" << ver << "', skipping\n";
        continue;
      }

      targets.push_back({pkg, ver, &available.at(pkg).at(ver), cfg, upd});
    }
    
    install(targets);
  } else {
    std::cerr << "error: command '" << args[1] << "' is not recognized, see '" << args[0] << " help' for more information\n";
    return 1;
  }

  return 0;
}

void mkdirs() {
  for (auto & dir : DIRS) {
    auto r = mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (r != 0 && errno != EEXIST) {
      std::cerr << "error: unrecoverable error in mkdir (" << errno << "), cannot continue\n";
      exit(1);
    }
  }
}

bool shcall(std::string const &command) {
  bool success = system(command.c_str()) == 0;
  if (!success) std::cerr << "error: execution of '" << command << "' returned non-zero exit code\n";
  return success;
}

void header(std::string const &str)
{
  if (str.length() >= 79)
    std::cout << str << "\n";
  else
    std::cout << "[" << std::string((78 - str.length()) / 2, ' ') << str << std::string((78 - str.length()) / 2 + str.length() % 2, ' ') << "]\n";
}

void require_root() {
  if (access("/", W_OK) != 0) {
    std::cerr << "error: root privilege is required\n";
    exit(1);
  }
}

void install(std::vector<target> const &targets) {
  json installed = access(DBI, F_OK) == 0 ? json::parse(std::ifstream(DBI)) : json::parse("{}");
  unlink(DBI);

  for (auto & target : targets) {
    header("installing '" + target.pkg + "'");
    
    std::string dir = DIRS.back() + "/" + target.pkg;
    std::string src = target.verinfo->at("src");
    std::string dst = target.verinfo->at("dst");
    dst = dir + std::string("/") + dst;
    std::string bld = target.verinfo->at("bld");

    if (mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
      std::cerr << "error: unrecoverable error in mkdir (" << errno << "), skipping\n";
      continue;
    }
    
    if (chdir(dir.c_str()) != 0) {
      std::cerr << "error: unrecoverable error in chdir (" << errno << "), skipping\n";
      continue;
    }

    char tmpsrc[] = {'j', 'p', 'w', '-', 's', 'r', 'c', 'X', 'X', 'X', 'X', 'X', 'X', '\0'};

    if (mkstemp(tmpsrc) == -1) {
      std::cerr << "error: unrecoverable error in mkstemp (" << errno << "), skipping\n";
      continue;
    }

    if (!curl::download(src, tmpsrc)) {
      std::cerr << "error: failed to retrieve source, skipping\n";
      continue;
    }
    
    if (bld == "gnu") {
      std::string worker = "jpw-worker-" + std::to_string(std::rand());

      if (!shcall("tar xf " + std::string(tmpsrc)))
        continue;

      unlink(tmpsrc);

      if (!shcall("useradd -M " + worker))
        continue;

      if (!shcall("chown -R " + worker + " " + dst))
        continue;

      if (chdir(dst.c_str()) != 0) {
        std::cerr << "error: failed to chdir\n";
        continue;
      }

      if (!shcall("su " + worker + " -c ./configure " + target.cfg))
        continue;

      if (!shcall("su " + worker + " -c make"))
        continue;

      if (!shcall("make install"))
        continue;

      shcall("su " + worker + " -c make clean");
      shcall("userdel -f " + worker);
      shcall("chown -R " + std::to_string(geteuid()) + " " + dst);
    } else {
      std::cerr << "error: build type '" << bld << "' is not yet implemented, skipping\n";
      continue;
    }

    installed[target.pkg] = {};
    installed[target.pkg]["ver"] = target.ver;
    installed[target.pkg]["cfg"] = target.cfg;
    installed[target.pkg]["upd"] = target.upd;
    installed[target.pkg]["bld"] = bld;
  }

  std::ofstream ofile (DBI);
  ofile << installed;
}

void update_available() {
  header("updating available package database");
  if (!curl::download("https://raw.githubusercontent.com/jonathanpwalton/jpw/main/available.json", DBA)) {
    std::cerr << "error: failed to retrieve available package database\n";
    exit(1);
  }
}

void update_installed() {
  std::cerr << "update_installed unimplemented\n";
  exit(1);
}
