#include "core.h"

int jpw::main_drop() {
	require(len(argv) && argv.pop() == "drop");
	require_permission();

	if (len(argv) == 0)
		error("one or more package names must be given");

	list<str> packages;

	while (len(argv)) {
		auto name = argv.pop();

		if (!fs::is_directory(lib_path / name) || !fs::is_directory(lib_path / name / "source") || !fs::is_regular_file(lib_path / name / "install") | !fs::is_regular_file(lib_path / name / "uninstall"))
			error("no such package: " + name);

		packages.append(name);
	}

	stage_beg("resolving dependencies");
	log("TODO");
	stage_end();

	auto cwd = fs::current_path();
	stage_beg("uninstalling packages");
	for (auto & package : packages) {
		stage_beg(package);
		fs::current_path(lib_path / package);

		if (!pipe("sh ./uninstall"))
			error("failed to uninstall");
		fs::remove_all(lib_path / package);
		stage_end();
	}
	stage_end();
	fs::current_path(cwd);

	TODO();
	return 0;
}