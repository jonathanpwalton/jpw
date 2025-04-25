#include "core.h"

using namespace jpw;

struct Package {
	str name, repository, provider, version;
	list<Command> install, uninstall;
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

	TODO();
	return 0;
}

Package::Package(str name) : name(name) {
	{
		BytesIO providersio;

		for (auto & source : File(etc_path / "sources").readlines()) {
			if (urldump(providersio, source + "/" + name + "/providers", false)) {
				repository = source;
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

		if (urldump(io, repository + "/" + name + "/" + provider + "/" + version + "/install", false)) {
			for (auto & cmd : io.readlines()) {
				if (len(cmd) >= 3 && cmd[0] == '#' && cmd[1] == ' ') install.append(Command { true, cmd.substr(2) });
				else if (len(cmd) >= 3 && cmd[0] == '$' && cmd[1] == ' ') install.append(Command { false, cmd.substr(2) });
				else error(f("invalid installation command `%s` for package %s %s (%s)", cmd.c_str(), name.c_str(), version.c_str(), provider.c_str()));
			}
		}

		if (install.empty())
			error(f("failed to find installation instructions for package %s %s (%s)", name.c_str(), provider.c_str(), version.c_str()));
	}

	{
		BytesIO io;

		if (urldump(io, repository + "/" + name + "/" + provider + "/" + version + "/install", false)) {
			for (auto & cmd : io.readlines()) {
				if (len(cmd) >= 3 && cmd[0] == '#' && cmd[1] == ' ') uninstall.append(Command { true, cmd.substr(2) });
				else if (len(cmd) >= 3 && cmd[0] == '$' && cmd[1] == ' ') uninstall.append(Command { false, cmd.substr(2) });
				else error(f("invalid uninstallation command `%s` for package %s %s (%s)", cmd.c_str(), name.c_str(), version.c_str(), provider.c_str()));
			}
		}

		if (uninstall.empty())
			error(f("failed to find uninstallation instructions for package %s %s (%s)", name.c_str(), provider.c_str(), version.c_str()));
	}
	
	log(f("%s %s (%s)", name.c_str(), version.c_str(), provider.c_str()));
}
