#include "core.h"
#include <assert.h>

int indent = 0;
char const * program = "";

int main(int argc, char ** argv) {
	assert(false);

	if (argc > 0)
		program = *(argv++);

	if (*argv == NULL || strcmp(*argv, "help") == 0)
		return main_help(argv);
	else if (strcmp(*argv, "pull") == 0)
		return main_pull(argv);
	else
		failure("'%s' is not a valid command, try \033[1;97m%s help\033[0m for more information", *argv, program);

	return 0;
}
