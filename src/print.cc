#include "core.h"

int jpw::main_help() {
	require(len(argv) == 0 || argv.pop() == "help");

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
			"  PACKAGE    \033[1;97mrequired\033[0m\n"
			"  :VERSION   \033[1;97moptional\033[0m\n"
			"  (PROVIDER) \033[1;97moptional\033[0m\n"
			"\n"
			"  PACKAGE, VERSION, and PROVIDER are all strings matching the following regex:\n"
			"    [A-Za-z0-9_.][A-Za-z0-9_.-]*\n"
			"\n"
			"  if VERSION is given, updates for the package are disabled\n"
			"  if PROVIDER is given, no prompt will be shown if more than one is available"
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
	require(len(argv) && argv.pop() == "list");

	for (const auto & e : fs::directory_iterator(lib_path)) {
		if (e.is_directory() && fs::is_regular_file(e.path() / "install") && fs::is_regular_file(e.path() / "uninstall") && fs::is_directory(e.path() / "source"))
			log(e.path().filename());
	}

	return 0;
}
