#include "core.h"

using namespace jpw;

struct Package {
	str name, repository, provider, version, sourceurl;
	list<str> install, uninstall;
	BytesIO source;
	Package(str name);
};

bool portably_posix(str const & n) {
	for (size_t i = 0; i < len(n); i++) {
		auto c = n[i];
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (i != 0 && c == '-') || c == '_' || c == '.'))
			return false;
	}
	return true;
}

int jpw::main_pull() {
	require_permission();
	require(argv.pop() == "pull");

	if (!fs::is_regular_file(etc_path / "sources")) {
		stage_beg("initializing default package repository sources");
		fs::remove_all(etc_path / "sources");
		File(etc_path / "sources", "w").write("https://raw.githubusercontent.com/jonathanpwalton/packages/refs/heads/main");
		stage_end();
	}

	stage_beg("checking for updates");
	stage_beg("TODO");
	stage_end();
	stage_end();

	if (argv.empty())
		return 0;

	stage_beg("resolving packages");

	list<str> package_names;
	while (len(argv)) {
		package_names.append(argv.pop());
		if (!portably_posix(package_names.back())) error(f("`%s` is not a valid package name", package_names.back().c_str()));
	}

	list<Package> packages;
	for (auto & name : package_names)
		packages.emplace_back(name);
	stage_end();

	stage_beg("resolving dependencies");
	stage_beg("TODO");
	stage_end();
	stage_end();

	stage_beg("downloading packages");
	for (auto & p : packages) {
		if (!urldump(p.source, p.sourceurl))
			error(f("failed to download source for package %s %s (%s)", p.name.c_str(), p.version.c_str(), p.provider.c_str()));
	}
	stage_end();

	stage_beg("extracting packages");
	{
		fs::remove_all(lib_path / ".tmp");
		require(fs::create_directories(lib_path / ".tmp"));
		Package * failed = nullptr;

		for (auto & p : packages) {
			auto top = lib_path / ".tmp" / p.name;
			auto dst = top / "source";

			if (!fs::create_directories(dst)) {
				failed = &p;
				break;
			}

			log(p.name + " ... ");
			if (!extract_archive(p.source, dst)) {
				log("\033[A\r\033[0K", "\r");
				log(p.name + " ... failed");
				failed = &p;
				break;
			}
			log("\033[A\r\033[0K", "\r");
			log(p.name + " ... done");

			File(top / "install", "w").write(p.install);
			File(top / "uninstall", "w").write(p.uninstall);
		}

		if (failed) {
			fs::remove_all(lib_path / ".tmp");
			error(f("failed to extract source", failed->name.c_str(), failed->version.c_str(), failed->provider.c_str()));
		}
	}
	stage_end();

	TODO();
	return 0;
}

Package::Package(str name) : name(name) {
	{
		BytesIO providersio;

		for (auto & repo : File(etc_path / "sources").readlines()) {
			if (urldump(providersio, repo + "/" + name + "/providers", false)) {
				repository = repo;
				break;
			}
		}

		if (repository.empty())
			error(f("failed to find package %s", name.c_str()));

		providersio.flush();
		auto providers = providersio.readlines();

		if (providers.empty())
			error(f("failed to find providers for package %s", name.c_str()));
		else if (len(providers) == 1)
			provider = providers[0];
		else
			TODO();
	}

	{
		BytesIO io;

		if (urldump(io, repository + "/" + name + "/" + provider + "/versions", false)) {
			auto versions = io.readlines();
			version = versions[0];
		}

		if (version.empty())
			error(f("failed to find versions for package %s (%s)", name.c_str(), provider.c_str()));
	}

	{
		BytesIO io;

		if (urldump(io, repository + "/" + name + "/" + provider + "/" + version + "/install", false))
			install = io.readlines();

		if (install.empty())
			error(f("failed to find install script for package %s %s (%s)", name.c_str(), provider.c_str(), version.c_str()));
	}

	{
		BytesIO io;

		if (urldump(io, repository + "/" + name + "/" + provider + "/" + version + "/install", false))
			uninstall = io.readlines();

		if (uninstall.empty())
			error(f("failed to find uninstall script for package %s %s (%s)", name.c_str(), provider.c_str(), version.c_str()));
	}

	{
		BytesIO io;

		if (urldump(io, repository + "/" + name + "/" + provider + "/" + version + "/source", false)) {
			auto url = io.readline();
			if (url) sourceurl = url.value;
		}

		if (sourceurl.empty())
			error(f("failed to find source for package %s %s (%s)", name.c_str(), version.c_str(), provider.c_str()));
	}
	
	log(f("%s %s (%s)", name.c_str(), version.c_str(), provider.c_str()));
}
