#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cstdlib>
#include <ctime>

#include "core.hpp"

void cmd::pull() {
  permit();

  begstage("synchronizing package repository");
  {
    srand(time(0));
    auto tmpdir = std::path("/tmp") / ("jpw-" + std::to_string(rand()));
    mkdirs(tmpdir);

    std::path dst = download("https://github.com/jonathanpwalton/packages/archive/refs/heads/main.tar.gz", cache, "core");

    if (system(("tar xf '" + dst.string() + "' -C " + tmpdir.string()).c_str()) != 0)
      throw error("failed to extract repository, ensure that 'tar' is installed and is capable of extracting '.tar.gz' files");

    std::filesystem::remove_all(repo);
    std::filesystem::copy(tmpdir / "packages-main", repo, std::filesystem::copy_options::recursive);
    std::filesystem::remove_all(tmpdir);
  }
  endstage();

  if (argv.empty())
    return;

  struct package {
    ssize_t priority = 0;
    std::string name, provider, version;
    std::string src, dst, bld;

    bool built = false;
    bool installed = false;
    std::vector<std::shared_ptr<package>> depends;

    package(std::string n, std::string const & p, std::string const & v) : name(n), provider(p), version(v) {
      auto s = repo / n;

      if (!is_regular_file(s / p / v / "src") || !is_regular_file(s / p / v / "dst") || !is_regular_file(s / p / v / "bld"))
        throw error("invalid package description in the repository, please clean and pull again");

      std::ifstream(s / p / v / "src") >> src;
      std::ifstream(s / p / v / "dst") >> dst;
      std::ifstream(s / p / v / "bld") >> bld;
    }
  };

  std::unordered_set<std::string> packages;
  std::vector<std::shared_ptr<package>> stages;

  while (!argv.empty()) {
    std::string pname = argv.front(); argv.pop();

    if (!is_directory(repo / pname))
      throw error(pname + " was not found in the package repostory");

    std::string pprov = "";
    if (!argv.empty() && argv.front() == "-p") {
      argv.pop();

      if (argv.empty())
        throw error("option -p was set for package " + pname + " but no provider was given");

      pprov = argv.front();
      argv.pop();

      if (!is_directory(repo / pname / pprov))
        throw error(pprov + " is not listed as a provider for " + pname);
    } else {
      auto it = iterate(repo / pname);
      std::vector<std::path> paths;

      for (auto & e : it)
        paths.push_back(e.path());

      if (paths.empty()) {
        throw error("no providers found for " + pname);
      } else if (paths.size() == 1) {
        pprov = paths[0].filename();
      } else {
        throw error("provider selection is not yet implemented");
      }
    }

    std::string pvers = "";
    if (!argv.empty() && argv.front() == "-v") {
      argv.pop();

      if (argv.empty())
        error("option -v was set for package " + pname + " but no version was given");

      pvers = argv.front();
      argv.pop();

      if (!is_directory(repo / pname / pprov / pvers))
        throw error(pvers + " is not listed as a version for " + pname + " from " + pprov);
    } else {
      if (!exists(repo / pname / pprov / "latest"))
        throw error("no latest version found for " + pname + " from " + pprov);

      std::ifstream(repo / pname / pprov / "latest") >> pvers;
    }

    if (packages.contains(pname))
      throw error(pname + " was given more than once");
    else
      packages.insert(pname);

    stages.emplace_back(std::make_shared<package>(pname, pprov, pvers));
  }

  begstage("resolving package dependencies");
  begstage("TODO");
  endstage();
  endstage();

  begstage("retrieving package sources");
  for (auto & s : stages) {
    download(s->src, cache, s->name);
  }
  endstage();
}

