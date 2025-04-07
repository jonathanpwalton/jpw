#include "core.hpp"

static size_t _indent;
static std::queue<std::string> _argv;
static std::path _root;
static std::path _cache;
static std::path _repo;
static std::path _store;

size_t & get_indent() { return _indent; }
std::queue<std::string> & get_argv() { return _argv; }
std::path const & get_root() { return _root; }
std::path const & get_repo() { return _repo; }
std::path const & get_cache() { return _cache; }
std::path const & get_store() { return _store; }

int main (int argc, char *cargv[]) {
  char selfa[PATH_MAX];
  ssize_t self_length = readlink("/proc/self/exe", selfa, PATH_MAX);

  try {
    if (self_length == -1)
      throw error("unrecoverable error in readlink");

    std::string self(selfa, selfa + self_length);

    if (!self.ends_with("/usr/local/bin/jpw"))
      throw error("this executable must be installed to */usr/local/bin/jpw");

    _root = self.substr(0, self.length() - 17);
    _cache = _root / "var/cache/jpw";
    _repo = _root / "var/lib/jpw/repository";
    _store = _root / "var/lib/jpw/store";

    for (int i = 1; i < argc; i++)
      _argv.push(cargv[i]);

    if (argv.size() == 0) {
      throw error("a command must be supplied, try 'jpw help' for more information");
    } else {
      auto command = argv.front(); argv.pop();

      if (command == "pull") {
        cmd::pull();
      } else if (command == "help") {
        cmd::help();
      } else {
        throw error("command '" + command + "' is not recognized, try 'jpw help' for more information");
      }
    }
  } catch (error & e) {
    std::cerr << std::string(INDENT, ' ') << "error: " << e.what() << "\n";
  }

  return 0;
}

