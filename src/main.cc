#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include <unistd.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <functional>
#include <curl/curl.h>
#include <cmath>
#include <archive.h>
#include <archive_entry.h>
#include <unordered_set>
#include <unordered_map>
#include <openssl/sha.h>
#include <sys/wait.h>
#include <string.h>
#include <ftw.h>

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)
#define todo() throw error(__FILE__ ":" STRINGIFY(__LINE__) ": unimplemented")
#define reverse(xs) std::make_reverse_iterator(xs)

using std::string;
using std::vector;
using std::unordered_set;
using std::unordered_map;

namespace fs = std::filesystem;
namespace std::filesystem { static bool chown(path const & p, uid_t owner, gid_t group); }
namespace env { vector<string> opts; vector<string> args; fs::path root = "/", conf, pkgs, temp, data; static void resolve_paths(); }
namespace cmd { static void needs_root(); static void update(); static void install(); }
namespace io {
  static size_t indent = 0;
  static void warn(string const & msg) { std::cerr << string(indent, ' ') << "\033[1;93mwarning: \033[0m" << msg << std::endl; }
  static void stage(string const & label) { std::cout << "\033[1;97m" << std::string(indent, ' ') << (indent > 0 ? "=> " : ":: ") << label << "\033[0m" << std::endl; indent += 3; }
  static void stage() { indent -= 3; }
  static void erase_previous_line() { std::cout << "\r\033[A\033[2K" << std::flush; }
  static void progress(string const & label, size_t now, size_t total, bool erase_prev = true);
  static bool exec(string const & cmd);
}

struct error : public std::runtime_error { error(string const & msg) : std::runtime_error(msg) {} };

template<typename X> static inline size_t len(vector<X> const & xs) { return xs.size(); }
template<typename X, class F> static vector<X> filter(vector<X> const & xs, F const & p) { vector<X> result; for (auto const & x : xs) if (p(x)) result.push_back(x); return result; }
template<typename X, typename Y, class F> static vector<Y> map(vector<X> const & xs, F const & f) { vector<Y> result; for (auto const & x : xs) result.push_back(f(x)); return result; }
static string ltrim(string const & s) { size_t o = 0; while (isspace(s[o])) o++; return string(s.begin() + o, s.end()); }
static string rtrim(string const & s) { size_t e = 0; while (isspace(s[s.length() - 1 - e])) e++; return string(s.begin(), s.end() - e); }
static inline string trim(string const & s) { return rtrim(ltrim(s)); }
static string tolower(string const & s) { string r; r.reserve(s.length()); for (auto & c : s) r.push_back(tolower(c)); return r; }

static void run();

int main(int argc, char ** argv) {
  for (int i = argc - 1; i >= 0; i--) if (!trim(argv[i]).empty()) { if (trim(argv[i]).starts_with("-")) env::opts.emplace_back(argv[i]); else env::args.emplace_back(argv[i]); }
  try {
    run();
  } catch (error const & e) {
    std::cerr << string(io::indent, ' ') << "\033[1;91merror: \033[0m" << e.what() << std::endl;
    exit(1);
  } catch (fs::filesystem_error const & e) {
    std::cerr << string(io::indent, ' ') << "\033[1;91merror: \033[0m" << tolower(e.code().message()) << " `" << (e.path2().empty() ? e.path1().string() : e.path1().string() + "` `" + e.path2().string()) << "`" << std::endl;
    exit(1);
  }
  return 0;
}

struct curl {
  curl(string const & url, std::basic_ostream<char> & dst) : handle(curl_easy_init()), url(url) {
    if (!handle)
      throw error("failed to initialize curl");
    set(CURLOPT_ERRORBUFFER, errorbuf);
    set(CURLOPT_URL, this->url.c_str());
    set(CURLOPT_FOLLOWLOCATION, true);
    set(CURLOPT_FAILONERROR, true);
    set(CURLOPT_WRITEDATA, &dst);
    set(CURLOPT_WRITEFUNCTION, curl::write);
    set(CURLOPT_XFERINFOFUNCTION, curl::progress);
    set(CURLOPT_XFERINFODATA, this->url.c_str());
    set(CURLOPT_USERAGENT, "Mozilla/5.0");
  }

  ~curl() {
    curl_easy_cleanup(handle);
  }

  template<typename V> bool set(CURLoption option, V const & value) {
    if (curl_easy_setopt(handle, option, value) != CURLE_OK)
      throw error("failed to set curl option `" + string(curl_easy_option_by_id(option)->name) + "` (" + errorbuf + ")");
    return true;
  }

  bool perform() {
    CURLcode code;
    do code = curl_easy_perform(handle); while (++performs <= 3 && code != CURLE_OK);
    return code == CURLE_OK;
  }

  string errstr() {
    return tolower(errorbuf);
  }

private:
  CURL * handle;
  string url;
  char errorbuf[CURL_ERROR_SIZE] = { 0 };
  size_t performs = 0;

  static size_t write(char * ptr, size_t, size_t nmemb, std::basic_ostream<char> * userdata) {
    auto pos = userdata->tellp();
    userdata->write(ptr, nmemb);
    return userdata->tellp() - pos;
  }

  static int progress(char const * clientp, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) {
    io::progress(clientp, now, total);
    return 0;
  }
};

struct extractor {
  extractor(std::string const & name, std::basic_istream<char> const & archive) : name(name), read(archive_read_new()), write(archive_write_disk_new()), buffer() {
    {
      std::stringstream ss;
      ss << archive.rdbuf();
      buffer = ss.str();
    }

    if (!read) throw error("failed to create new read archive");
    if (archive_read_support_format_all(read) || archive_read_support_filter_all(read) ||
      archive_read_open_memory(read, buffer.data(), buffer.length() * sizeof(char)))
      throw error("read archive error (" + tolower(archive_error_string(read)) + ")");

    struct archive_entry * entry;
    while (archive_read_next_header(read, &entry) == ARCHIVE_OK)
      total += archive_entry_size_is_set(entry) ? archive_entry_size(entry) : 0;

    archive_read_close(read);
    archive_read_free(read);

    read = archive_read_new();
    if (!read) throw error("failed to create new read archive");
    if (archive_read_support_format_all(read) || archive_read_support_filter_all(read) ||
      archive_read_open_memory(read, buffer.data(), buffer.length() * sizeof(char)))
      throw error("read archive error (" + tolower(archive_error_string(read)) + ")");

    if (!write) throw error("failed to create new write archive");
    if (archive_write_disk_set_options(write, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_UNLINK | ARCHIVE_EXTRACT_SECURE_NODOTDOT) ||
      archive_write_disk_set_standard_lookup(write))
      throw error("write archive error (" + tolower(archive_error_string(write)) + ")");
  }

  bool extract() {
    int r;
    struct archive_entry * entry;

    io::progress(name, 0, total, false);
    for (;;) {
      r = archive_read_next_header(read, &entry);
      if (r == ARCHIVE_EOF) break;
      if (r < ARCHIVE_OK) errorstr = archive_error_string(read);
      if (r < ARCHIVE_WARN) return false;

      r = archive_write_header(write, entry);
      if (r < ARCHIVE_OK) errorstr = archive_error_string(write);
      else if (archive_entry_size(entry) > 0) {
        now += archive_entry_size(entry);
        io::progress(name, now, total);
        r = copy();
        if (r < ARCHIVE_OK) errorstr = archive_error_string(write);
        if (r < ARCHIVE_WARN) return false;
      }

      r = archive_write_finish_entry(write);
      if (r < ARCHIVE_OK) errorstr = archive_error_string(write);
      if (r < ARCHIVE_WARN) return false;
    }

    return true;
  }

  ~extractor() {
    archive_read_close(read);
    archive_read_free(read);
    archive_write_close(write);
    archive_write_free(write);
  }

  string errstr() {
    return tolower(errorstr);
  }

private:
  string name;
  struct archive * read;
  struct archive * write;
  string buffer = "";
  string errorstr = "";
  size_t now = 0, total = 0;

  int copy() {
    int res;
    const void * buf;
    size_t len;
    la_int64_t off;

    for (;;) {
      res = archive_read_data_block(read, &buf, &len, &off);
      if (res == ARCHIVE_EOF) return ARCHIVE_OK;
      if (res < ARCHIVE_OK) return res;

      res = archive_write_data_block(write, buf, len, off);
      if (res < ARCHIVE_OK) return res;
    }

    return ARCHIVE_OK;
  }
};

void run() {
  if (len(env::args) == 0)
    throw error("invalid program invocation");
  else
    env::args.pop_back();

  for (auto opt = env::opts.rbegin(); opt != env::opts.rend(); opt++) {
    if (opt->starts_with("-r")) {
      if (!opt->starts_with("-r=") || opt->length() < 4)
        throw error("option `-r` requires a value in the form `-r=PATH`");
      env::root = opt->substr(3);
    } else {
      throw error("unknown option `" + *opt + "`");
    }
  }

  env::resolve_paths();

  if (len(env::args) == 0)
    throw error("expected a command");

  string cmd = env::args.back();
  env::args.pop_back();

  if (cmd == "update")
    cmd::needs_root(), cmd::update();
  else if (cmd == "install")
    cmd::needs_root(), cmd::install();
  else
    throw error("unknown command `" + cmd + "`");
}

void env::resolve_paths() {
  root = fs::canonical(root);
  if (!fs::is_directory(root))
    throw error("invalid root directory `" + root.string() + "` (not a directory)");
  conf = root / "etc" / "jpw";
  pkgs = root / "var" / "lib" / "jpw" / "packages";
  data = root / "var" / "lib" / "jpw" / "database";
  temp = root / "var" / "tmp" / "jpw";
}

void cmd::needs_root() {
  if (geteuid() != 0)
    throw error("you must be root to perform this action");

  fs::remove_all(env::temp);

  for (auto const & path : { env::conf, env::pkgs, env::temp, env::data })
    fs::create_directories(path);
}

void cmd::update() {
  if (len(env::args))
    throw error("this command does not take any arguments");

  if (!fs::exists(env::conf / "repositories"))
    std::ofstream(env::conf / "repositories") << "jonathanpwalton packages" << std::endl;

  struct Repository { string user, name, tarballurl; std::stringstream tarball = std::stringstream(); bool retrieved = false; };
  vector<Repository> repos;
  unordered_set<string> repoNames;

  {
    auto reposFile = std::ifstream(env::conf / "repositories");
    Repository repo;
    while (reposFile >> repo.user >> repo.name)
      if (!repoNames.contains(repo.user + "/" + repo.name)) {
        repos.push_back({
          repo.user, repo.name, "https://api.github.com/repos/" + repo.user + "/" + repo.name + "/" + "tarball",
          });
        repoNames.insert(repo.user + "/" + repo.name);
      }
  }

  if (repos.empty())
    throw error("failed to update local database according to `" + (env::conf / "repositories").string() + "` (no repositories are configured)");

  size_t cnt = 0;
  io::stage("retrieving repositories");
  for (auto & repo : repos) {
    auto label = repo.user + "/" + repo.name;
    curl req(repo.tarballurl, repo.tarball);
    req.set(CURLOPT_NOPROGRESS, false);
    req.set(CURLOPT_XFERINFODATA, label.c_str());
    io::progress(label, 0, 0, false);
    if (!req.perform()) {
      io::erase_previous_line();
      io::warn("failed to retrieve repository `" + repo.user + "/" + repo.name + "` (" + req.errstr() + ")");
    } else {
      repo.retrieved = true;
      cnt++;
    }
  }
  io::stage();

  if (cnt == 0)
    return;

  fs::remove_all(env::data);
  fs::create_directories(env::data);

  io::stage("extracting repositories");
  for (auto & repo : repos) {
    if (!repo.retrieved)
      continue;

    auto cwd = fs::current_path();
    fs::remove_all(env::temp);
    fs::create_directories(env::temp);
    fs::current_path(env::temp);

    extractor ext(repo.user + "/" + repo.name, repo.tarball);

    if (!ext.extract()) {
      io::erase_previous_line();
      io::warn("failed to extract repository `" + repo.user + "/" + repo.name + "` (" + ext.errstr() + ")");
    } else for (auto & package : fs::directory_iterator(*fs::directory_iterator(env::temp))) {
      if (package.is_directory() && !fs::exists(env::data / package.path().filename()))
        fs::rename(package.path(), env::data / package.path().filename());
    }

    fs::current_path(cwd);
  }
  io::stage();
  fs::remove_all(env::temp);
}

struct Recipe {
  string package, vendor, version;
  struct { string type, url, sha256; } source = {};
  struct { string configure, build; } commands = {};
};

static unordered_map<string, unordered_map<string, vector<Recipe>>> read_database() {
  unordered_map<string, unordered_map<string, vector<Recipe>>> db;
  for (auto const & package : fs::directory_iterator(env::data)) {
    db[package.path().filename()] = {};

    for (auto const & vendor : fs::directory_iterator(package.path())) {
      db[package.path().filename()][vendor.path().filename()] = {};

      std::ifstream versions(vendor.path() / "versions");
      string version;
      while (std::getline(versions, version)) {
        std::ifstream file(vendor.path() / version);

        if (!file)
          continue;

        auto & recipe = db[package.path().filename()][vendor.path().filename()].emplace_back(
          package.path().filename(), vendor.path().filename(), version
        );

        if (!std::getline(file, recipe.source.type) || !std::getline(file, recipe.source.url) || !std::getline(file, recipe.source.sha256) ||
          !std::getline(file, recipe.commands.configure) || !std::getline(file, recipe.commands.build)) {
          db[package.path().filename()][vendor.path().filename()].pop_back();
          continue;
        }
      }

      if (db[package.path().filename()][vendor.path().filename()].empty())
        db[package.path().filename()].erase(vendor.path().filename());
    }

    if (db[package.path().filename()].empty())
      db.erase(package.path().filename());
  }
  return db;
}

static string sha256sum(void const * data, size_t len) {
  static unsigned char buf[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char const *) data, len, buf);
  string r; char hex[3];
  for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    snprintf(hex, 3, "%02x", buf[i]);
    r += hex;
  }
  return r;
}

void cmd::install() {
  if (env::args.empty())
    throw error("expected one or more package names");

  struct Target {
    string name;
    Recipe const & recipe;
    std::stringstream source;

    Target(string name, Recipe const & recipe) : name(name), recipe(recipe), source() {}
  };

  auto const db = read_database();
  unordered_set<string> targetNames;
  vector<vector<Target>> targets;

  io::stage("resolving package recipes");
  for (ssize_t i = len(env::args) - 1; i >= 0; i--) {
    auto & package = env::args[i];
    if (!db.contains(package)) {
      io::warn("skipping unavailable package `" + package + "`");
      continue;
    } else if (targetNames.contains(package)) {
      io::warn("skipping already targeted package `" + package + "`");
      continue;
    }

    string vendor;
    if (db.at(package).size() > 1) {
      todo();
    } else {
      vendor = db.at(package).begin()->first;
    }

    targets.push_back({});
    targets.back().emplace_back(Target(package, db.at(package).at(vendor).back()));
    targetNames.insert(package);
  }
  io::warn("TODO: request user permission to proceed");
  io::stage();

  if (targets.empty())
    return;

  io::stage("retrieving package sources");
  for (auto & targetChain : targets) {
    for (auto & target : targetChain) {
      auto label = target.recipe.source.url;
      if (label.rfind('/')) label = label.substr(label.rfind('/') + 1);
      curl req(target.recipe.source.url, target.source);
      req.set(CURLOPT_NOPROGRESS, false);
      req.set(CURLOPT_XFERINFODATA, label.c_str());
      io::progress(target.recipe.source.url, 0, 0, false);
      if (!req.perform()) {
        io::erase_previous_line();
        io::warn("failed to retrieve source for package `" + target.name + "`");
        break;
      } else if (target.recipe.source.sha256 != sha256sum(target.source.str().data(), target.source.str().length())) {
        io::warn("skipping package `" + target.name + "` due to source checksum mismatch");
        break;
      }
    }
  }
  io::stage();

  struct PackageEntry {
    string name, vendor, version;
    vector<fs::path> files;
  };

  unordered_set<fs::path> directoriesToCreate;
  unordered_map<fs::path, fs::path> filesToRename;
  vector<PackageEntry> packageEntries;

  fs::remove_all(env::temp);
  io::stage("building packages");
  for (auto & targetChain : targets) {
    for (auto & target : targetChain) {
      io::stage(target.name);
      auto cwd = fs::current_path();
      fs::create_directories(env::temp / target.name);
      fs::current_path(env::temp / target.name);
      if (target.recipe.source.type == "tarball") {
        target.source.clear();
        target.source.seekg(0);
        extractor ext("extracting source", target.source);

        if (!ext.extract()) {
          io::warn("failed to extract source tarball (" + ext.errstr() + ")");
          io::stage();
          fs::current_path(cwd);
          break;
        }
        fs::current_path(fs::directory_iterator(fs::current_path())->path());
      }

      fs::chown(env::temp / target.name, 1, 1);

      if (!io::exec(target.recipe.commands.configure)) {
        io::warn("failed to configure package");
        io::stage();
        fs::current_path(cwd);
        break;
      }

      if (!io::exec(target.recipe.commands.build)) {
        io::warn("failed to build package");
        io::stage();
        fs::current_path(cwd);
        break;
      }

      fs::current_path("./JPW_DESTDIR");

      auto & entry = packageEntries.emplace_back();
      entry.name = target.name;
      entry.vendor = target.recipe.vendor;
      entry.version = target.recipe.version;

      for (auto & f : fs::recursive_directory_iterator(fs::current_path())) {
        auto src = f.path();
        auto dst = fs::relative(src, fs::current_path());

        if (f.is_directory()) {
          directoriesToCreate.insert(env::root / dst);
        } else if (f.is_regular_file()) {
          filesToRename[src] = env::root / dst;
          entry.files.push_back(dst);
        } else {
          io::warn("refusing to copy " + src.string() + " as it is not a regular file");
        }
      }

      fs::current_path(cwd);
      io::stage();
    }
  }
  io::stage();

  io::stage("installing package files");
  for (auto const & dir : directoriesToCreate)
    fs::create_directories(dir);

  for (auto && [src, dst] : filesToRename)
    fs::rename(src, dst);

  for (auto const & entry : packageEntries) {
    io::stage(entry.name);
    fs::create_directories(env::pkgs / entry.name);
    std::ofstream meta(env::pkgs / entry.name / "meta");
    std::ofstream files(env::pkgs / entry.name / "files");

    meta << entry.name << '\n' << entry.vendor << '\n' << entry.version << '\n';

    for (auto const & file : entry.files)
      files << file.string() << '\n';
    io::stage();
  }
  io::stage();
}

void io::progress(string const & label, size_t now, size_t total, bool erase_prev) {
  if (erase_prev) erase_previous_line();

  static auto const bytes = [](size_t b) -> string {
    if (b == 0) return "   0B";
    static char const us[] = { 'B', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
    size_t p = floor(log(b) / log(1024));
    char u = us[p];
    double n = double(b) / double(pow(1024, p));
    char c[6];
    if (n >= 100) snprintf(c, 6, "%4.0f%c", n, u);
    else if (n >= 10) snprintf(c, 6, "%2.1f%c", n, u);
    else snprintf(c, 6, "%1.2f%c", n, u);
    return c;
    };

  char line[81] = { 0 };

  if (total > 0) {
    double pct = (double) now / total;
    int lwidth = 32 - indent;
    snprintf(line, 81, "%*s%-*.*s %3.0f%% [%-28.*s] %s %s", (int) indent, "", lwidth, lwidth, label.c_str(), pct * 100.0, int(pct * 28.0), "############################", bytes(now).c_str(), bytes(total).c_str());
  } else {
    int lwidth = 74 - indent;
    snprintf(line, 81, "%*s%-*.*s %s", (int) indent, "", lwidth, lwidth, label.c_str(), bytes(now).c_str());
  }

  std::cout << line << std::endl;
}

bool io::exec(string const & cmd) {
  int pipefd[2];
  pipe(pipefd);

  switch (fork()) {
    case -1: {
      throw error("failed to fork");
    } break;
    case 0: {
      setuid(1);
      close(pipefd[0]);
      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
      close(pipefd[1]);
      execl("/bin/sh", "sh", "-e", "-c", cmd.c_str(), NULL);
      throw error("failed to execute shell command " + cmd);
    } break;
    default: {
      close(pipefd[1]);
      FILE * file = fdopen(pipefd[0], "r");

      char * buf = nullptr;
      size_t len;

      std::cout << string(indent, ' ') << cmd << std::endl;
      while (getline(&buf, &len, file) > 0) {
        if (!trim(buf).empty()) std::cout << string(indent, ' ') << buf << std::flush;
        free(buf);
        buf = nullptr;
      }
      free(buf);

      int status;
      wait(&status);
      fclose(file);

      return status == 0;
    } break;
  }
}

static uid_t chown_walk_owner;
static gid_t chown_walk_group;

int chown_walk(const char * fpath, const struct stat *, int, struct FTW *) {
  return chown(fpath, chown_walk_owner, chown_walk_group);
}

bool std::filesystem::chown(path const & p, uid_t owner, gid_t group) {
  chown_walk_owner = owner;
  chown_walk_group = group;
  return nftw(p.c_str(), chown_walk, 64, FTW_PHYS | FTW_DEPTH) == 0;
}