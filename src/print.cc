#include "core.h"

int jpw::main_help() {
	print(f(
		"usage: %s [<command>]\n"
		"\n"
		"commands:\n"
		"  pull [<package> ...]           update and install packages\n"
		"  help [<command>]               see program and command usage information"
	, program.data()));
	return 0;
}
