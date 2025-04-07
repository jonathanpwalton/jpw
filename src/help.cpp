#include "core.hpp"

void cmd::help() {
  if (!argv.empty())
    throw error("the 'help' command does not take any arguments");

  std::cout <<
    "usage: jpw <command>\n" <<
    "\n" <<
    "commands:\n" <<
    "  pull [<package> [-p <provider>] [-v <version>] ...]     update local repository and install packages\n" <<
    "  drop <package> ...                                     uninstall and remove packages\n" <<
    "  show {available | installed}                           show all available or installed packages\n" <<
    "  help                                                   display this help text and quit\n";
}
