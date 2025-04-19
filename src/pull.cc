#include "jpw.h"

#include <unordered_set>
#include <deque>

namespace jpw {
	using std::unordered_set;
	using std::deque;

	struct Directive {
		bool root;
		string command;

		Directive() {}
		Directive(bool root, string command) : root(root), command(command) {}
	};

	struct Version {
		string name, source;
		vector<Directive> install;
		vector<Directive> uninstall;

		Version(queue<string> & lines);
	};

	struct Provider {
		string name;
		vector<Version> versions;

		Provider(queue<string> & lines);
	};

	struct Package {
		string name;
		vector<Provider> providers;

		string repo;
		Provider const * provider;
		Version const * version;
		bool update = true;

		Path folder;

		Package(string const & name);
		void select_provider(string const & name);
		void select_version(string const & name);
	};
}

void jpw::pull() {
	argv.pop();
	permit();

	if (!fs::is_regular_file(conf / "repositories")) {
		stage_beg("initializing package repositories");
		fs::remove_all(conf / "repositories");
		File(conf / "repositories", "w").writeln("https://raw.githubusercontent.com/jonathanpwalton/packages/refs/heads/main");
		stage_end();
	}

	if (fs::entry_count(pkgs) > 0) {
		stage_beg("updating packages");
		stage_beg("TODO");
		stage_end();
		stage_end();
	}

	if (argv.empty())
		return;

	unordered_set<string> package_names;
	deque<Package> packages;

	stage_beg("resolving packages");
	while (!argv.empty()) {
		stage_beg(argv.front());
		auto name = argv.front();
		argv.pop();

		Package * package;

		if (package_names.contains(name))
			throw ProgramError("unimpl");
		else
			package = &packages.emplace_back(name);

		if (!argv.empty() && argv.front() == "-p") {
			argv.pop();

			if (argv.empty())
				throw ProgramError("the provider option (-p) was set, but no provider was given");

			package->select_provider(argv.front());
			argv.pop();
		}
		//printf("%*sprovider %s\n", indent, "", package->provider->name.c_str());

		if (!argv.empty() && argv.front() == "-v") {
			argv.pop();

			if (argv.empty())
				throw ProgramError("the version option (-v) was set, but no version was given");

			package->select_version(argv.front());
			package->update = false;
			argv.pop();
		}

		//printf("%*sversion  %s\n", indent, "", package->version->name.c_str());
		stage_end();
	}
	stage_end();

	stage_beg("requesting package sources");
	for (auto & package : packages) {
		fs::remove_all(temp / package.name);
		fs::create_directories(temp / package.name);

		File file;
		download(package.version->source, file);
		extract(file, temp / package.name);

		if (fs::entry_count(temp / package.name) != 1) {
			fs::remove_all(temp / package.name);
			throw ProgramError("invalid source archive");
		}

		package.folder = fs::directory_iterator(temp / package.name)->path();
	}
	stage_end();

	stage_beg("building package sources");
	for (auto & package : packages) {
		stage_beg("building " + package.name);

		chown(package.folder, 1);
		for (auto & directive : package.version->install) {
			if (fork()) {
				fs::current_path(package.folder);
				if (!directive.root) setuid(1);
				exit(pipe(directive.command));
			}

			if (wait() != 0) {
				fs::remove_all(temp / package.name);
				throw ProgramError("failed to execute installation command");
			}
		}
		chown(package.folder, getuid());

		fs::remove_all(pkgs / package.name);
		fs::copy(temp / package.name, pkgs / package.name, fs::copy_options::recursive);

		File meta(pkgs / package.name / ".metadata", "w");
		meta.writeln(package.repo);
		meta.writeln(package.provider->name);
		meta.writeln(package.version->name);
		meta.writeln(package.update ? "update" : "do not update");
		for (auto & u : package.version->uninstall) meta.writeln((u.root ? "# " : "$ ") + u.command);

		fs::remove_all(temp / package.name);
		stage_end();
	}
	stage_end();
}

jpw::Package::Package(string const & name) : name(name) {
	File package;

	for (auto & repository : File(conf / "repositories", "r").readlines()) try {
		download(repository + "/" + name, package, false);
		repo = repository;
		break;
	} catch (ProgramError &) {}

	if (package.length == 0)
		throw ProgramError("failed to find package");

	queue<string> lines;
	for (auto & line : package.readlines())
		lines.push(line);

	while (!lines.empty() && !lines.front().starts_with("\t"))
		providers.emplace_back(lines);

	if (providers.empty())
		throw ProgramError("failed to find provider");

	provider = &providers.front();

	if (provider->versions.empty())
		throw ProgramError("failed to find version");

	version = &provider->versions.front();
}

void jpw::Package::select_provider(string const & name) {
	for (auto const & p : providers)
		if (p.name == name) {
			provider = &p;
			return;
		}
	throw ProgramError("failed to find provider " + name);
}

void jpw::Package::select_version(string const & name) {
	for (auto const & v : provider->versions)
		if (v.name == name) {
			version = &v;
			return;
		}
	throw ProgramError("failed to find version " + name);
}

jpw::Provider::Provider(queue<string> & lines) : name(lines.front()) {
	lines.pop();

	while (!lines.empty() && lines.front().size() >= 2 && lines.front().starts_with('\t') && lines.front()[1] != '\t')
		versions.emplace_back(lines);
}

jpw::Version::Version(queue<string> & lines) : name(lines.front().substr(1)) {
	lines.pop();

	if (lines.empty() || lines.front().size() < 3 || !lines.front().starts_with("\t\t") || lines.front()[2] == '\t')
		throw ProgramError("failed to find version source");

	source = lines.front().substr(2);
	lines.pop();

	if (lines.empty() || lines.front().size() < 3 || !lines.front().starts_with("\t\t") || lines.front()[2] == '\t')
		throw ProgramError("failed to find version build type");

	auto build = lines.front().substr(2);
	lines.pop();

	if (build == "autoconf-makefile") {
		install.emplace_back(false, "./configure");
		install.emplace_back(false, "make");
		install.emplace_back(true, "make install");

		uninstall.emplace_back(true, "make uninstall");
	} else if (build == "custom") {
		if (lines.empty() || lines.front() != "\t\t\tinstall")
			throw ProgramError("failed to find version install instructions for custom build");
		lines.pop();

		while (!lines.empty() && lines.front().starts_with("\t\t\t\t")) {
			auto & line = lines.front();

			if (line.starts_with("\t\t\t\t$ "))
				install.emplace_back(false, line.substr(6));
			else if (line.starts_with("\t\t\t\t# "))
				install.emplace_back(true, line.substr(6));
			else
				throw ProgramError("invalid installation command " + line.substr(4));

			lines.pop();
		}

		if (lines.empty() || lines.front() != "\t\t\tuninstall")
			throw ProgramError("failed to find version uninstall instructions for custom build");
		lines.pop();

		while (!lines.empty() && lines.front().starts_with("\t\t\t\t")) {
			auto & line = lines.front();

			if (line.starts_with("\t\t\t\t$ "))
				uninstall.emplace_back(false, line.substr(6));
			else if (line.starts_with("\t\t\t\t# "))
				uninstall.emplace_back(true, line.substr(6));
			else
				throw ProgramError("invalid uninstallation command " + line.substr(4));

			lines.pop();
		}
	} else {
		throw ProgramError("unknown build type " + build);
	}

	if (install.empty())
		throw ProgramError("failed to find version install instructions");

	if (uninstall.empty())
		throw ProgramError("failed to find version uninstall instructions");
}