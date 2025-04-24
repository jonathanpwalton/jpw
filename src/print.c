#include "core.h"

int main_help(char ** argv) {
	require(*argv == NULL || strcmp(*(argv++), "help") == 0);
	fprintf(stdout,
		"usage: %s <command>\n"
		"\n"
		"commands:\n"
		"  help [<command>]               get program or command usage help\n"
	, program);
	return 0;
}
