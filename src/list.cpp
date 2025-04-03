#include "jpw.hpp"
#include "json.hpp"

void list::available() {
  error("the available package database has not been initialized, run '" + argv[0] + " update available' first", access((prefix + "/available.json").c_str(), F_OK) != 0);

  auto db = nlohmann::json::parse(std::ifstream(prefix + "/available.json"));
  for (auto && [key, value] : db.items())
    std::cout << key << "\n";
}

void list::installed() {
  if (!exists(packages)) return;
  chdir(packages.c_str());
  system("dir --width=1");
}
