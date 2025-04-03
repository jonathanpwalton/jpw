#include "jpw.hpp"
#include "json.hpp"

#define option(OPT, VAR) std::string VAR = ""; do { \
  if (i < argv.size() && argv[i] == OPT) { \
    error("the option " OPT " must be followed by a string", ++i == argv.size()); \
    VAR = argv[i++]; \
  } \
} while (0)

void install() {
  error("the 'install' command requires write permisson", !permitted());
  error("no packages available, run '" + argv[0] + " update available' first", !exists(prefix + "/available.json"));

  struct Package {
    std::string name;
    nlohmann::json & version;
    bool update;
    std::string config;
  };

  auto available = nlohmann::json::parse(std::ifstream(prefix + "/available.json"));
  std::vector<Package> staged;

  for (size_t i = 2; i < argv.size();) {
    std::string package = argv[i++];
    option("-p", provider);
    option("-v", version);
    option("-c", config);
#undef option
    bool update = version == "";

    error("cannot install unavailable package '" + package + "'", !available.contains(package));

    if (provider.empty())
      provider = available[package].begin().key();
    else
      error("cannot install package '" + package + "' from unavailable provider '" + provider + "'", !available[package].contains(provider));

    if (version.empty())
      version = available[package][provider].begin().key();
    else
      error("cannot install unavailable version '" + version + "' of package '" + package + "' from provider '" + provider + "'", !available[package][provider].contains(version));

    staged.emplace_back(Package{ package, available[package][provider][version], update, config });
  }

  for (auto & package : staged) {
    if (!exists(cache + "/" + basename(package.version["src"]))) {
      error("failed to create cache directory", !exists(cache) && !mkdir(cache));
      error("failed to change to cache directory", !chdir(cache));
      error("failed to download source for package '" + package.name + "'", !download(package.version["src"]));
    }

    if (!exists(packages))
      error("faiiled to create packages directory", !mkdir(packages));

    error("failed to change to packages directory", !chdir(packages));
    error("failed to create directory for package '" + package.name + "'", !mkdir(packages + "/" + package.name));
    error("failed to change to directory for package '" + package.name + "'", !chdir(packages + "/" + package.name));

    std::ofstream info("package.json");
    info << package.version;

    if (package.version["bld"] == "gnu") {
      error("failed to extract source for package '" + package.name + "'", system(("tar xf " + cache + "/" + basename(package.version["src"])).c_str()) != 0);
      error("failed to change directory", !chdir(packages + "/" + package.name + "/" + std::string(package.version["dst"])));

      switch (fork()) {
        case -1:
          error("failed to fork");
          break;
        case 0:
          unsigned short uid = 10000 + rand() % 30000;
          error("failed to change owner", system(("chown -R " + std::to_string(uid) + " " + packages + "/" + package.name).c_str()) != 0);
          error("failed to set user id", setuid(uid) != 0);
          error("failed to configure", system(("./configure " + package.config).c_str()) != 0);
          error("failed to make", system("make") != 0);
          exit(0);
          break;
      }
      int status;
      wait(&status);
      if (status != 0) exit(1);

      error("failed to change owner", system(("chown -R " + std::to_string(geteuid()) + " " + packages + "/" + package.name).c_str()) != 0);
      error("failed to install", system("make install") != 0);
      system("make clean");
    } else {
      error("build '" + std::string(package.version["bld"]) + "' is unimplemented");
    }
  }
}