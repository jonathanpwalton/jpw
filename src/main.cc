#define JPW_IMPLEMENTATION
#include "core.h"
#include <unistd.h>

using namespace jpw;

int main(int argc, char ** cargv) {
	if (argc > 0) program = cargv[0];
	for (int i = argc - 1; i >= 1; i--) argv.append(cargv[i]);

	set_root("/");

	if (len(argv) == 0 || argv.back() == "help")
		return main_help();
	else if (argv.back() == "pull")
		return main_pull();
	else
		error(f("'%s' is not a valid command, try '%s help' for more information", argv.back().data(), program.data()));

	return 1;
}

void jpw::set_root(Path root) {
	if (!fs::is_directory(root))
		error(f("no such directory %s", root.c_str()));

	root_path = root;
	etc_path = root / "etc/jpw";
	lib_path = root / "var/lib/jpw";
}

void jpw::require_permission() {
	require(!root_path.empty());

	try {
		if (!fs::is_directory(etc_path)) fs::remove(etc_path);
		if (!fs::is_directory(lib_path)) fs::remove(lib_path);
		fs::create_directories(etc_path);
		fs::create_directories(lib_path);
	
		for (auto & path : {root_path, etc_path, lib_path}) {
			if (access(path.c_str(), F_OK) != 0 || access(path.c_str(), R_OK | W_OK | X_OK) != 0)
				throw fs::filesystem_error("", std::make_error_code(std::errc::permission_denied));
		}
	} catch (fs::filesystem_error & e) {
		if (e.code().value() != (int) std::errc::permission_denied)
			throw e;

		error("you do not have permission to perform this action");
	}
}