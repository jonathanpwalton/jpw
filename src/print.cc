#include "core.h"

int jpw::main_help() {
	require(argv.pop() == "help");

	if (argv.empty())
		print(f(
			"usage: %s [<command>]\n"
			"\n"
			"commands:\n"
			"  pull [<package> ...]           update and install packages\n"
			"  list                           list installed packages\n"
			"  help [<command>]               see program and command usage information"
		, program.c_str()));
	else if (len(argv) == 1 && argv.back() == "pull")
		print(f(
			"update and install packages\n"
			"\n"
			"usage: %s pull [<package> ...]\n"
			"\n"
			"package format: PACKAGE:VERSION(PROVIDER)\n"
			"  'PACKAGE' is a \033[1;97mrequired\033[0m field, where PACKAGE is a string conformant to\n"
			"            POSIX's 'fully portable filenames' specification\n"
			"  ':VERSION' is an \033[1;97moptional\033[0m field, where VERSION is a POSIX fully portable\n"
			"            filename and the provision of this field disables the inclusion of\n"
			"            this package when checking for available updates\n"
			"  '(PROVIDER)' is an \033[1;97moptional\033[0m field, where PROVIDER is a POSIX fully portable\n"
			"            filename; this option selects a provider for a specific package"
		, program.c_str()));
	else if (len(argv) == 1 && argv.back() == "list")
		print(f(
			"list installed packages\n"
			"\n"
			"usage: %s list"
		, program.c_str()));
	else
		error(f("unknown command, try '%s help' for more information", program.c_str()));

	return 0;
}

int jpw::main_list() {
	require(argv.pop() == "list");

	for (const auto & e : fs::directory_iterator(lib_path)) {
		if (e.is_directory() && fs::is_regular_file(e.path() / "install") && fs::is_regular_file(e.path() / "uninstall") && fs::is_directory(e.path() / "source"))
			log(e.path().filename());
	}

	return 0;
}
