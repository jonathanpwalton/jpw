#include "core.hpp"

void cmd::help() {
  if (!argv.empty())
    throw error("the 'help' command does not take any arguments");

  std::cout << R"(usage: jpw <command>

commands:
  pull [<package> [-p <provider>] [-v <version>] ...]    update local repository and install packages
  drop <package> ...                                     uninstall and remove packages
  show {available | installed}                           show all available or installed packages
  help                                                   display this help text and quit
)" << std::flush;
}
