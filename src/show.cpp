#include "core.hpp"

void cmd::show() {
  if (argv.size() != 1 || (argv.front() != "available" && argv.front() != "installed")) {
    throw error("the 'show' command takes exactly one argument of {available | installed}");
  } else if (argv.front() == "available") {
    argv.pop();

    if (!exists(repo))
      return;

    for (auto & package : iterate(repo))
      if (is_directory(package.path()))
        std::cout << package.path().filename().string() << '\n';
    std::cout << std::flush;
  } else if (argv.front() == "installed") {
    argv.pop();

    if (!exists(store))
      return;

    for (auto & package : iterate(store))
      if (is_directory(package.path()))
        std::cout << package.path().filename().string() << '\n';
    std::cout << std::flush;
  } else {
    throw error("unreachable");
  }
}
