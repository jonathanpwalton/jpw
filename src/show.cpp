#include "core.hpp"

void cmd::show() {
  if (argv.size() == 1 && argv.front() == "available") {
    argv.pop();

    if (!exists(repo) || !is_directory(store))
      return;

    for (auto & package : iterate(repo))
      if (is_directory(package.path()))
        std::cout << package.path().filename().string() << '\n';
    std::cout << std::flush;
  } else if (argv.size() == 1 && argv.front() == "installed") {
    argv.pop();

    if (!exists(store) || !is_directory(store))
      return;

    for (auto & package : iterate(store))
      if (is_directory(package.path()))
        std::cout << package.path().filename().string() << '\n';
    std::cout << std::flush;
  } else {
    throw error("the 'show' command takes exactly one argument of {available | installed}");
  }
}
