#include "jpw.h"
#include <stdio.h>

void jpw::help() {
	printf(
		"usage: jpw <command>\n"
		"\n"
		"commands:\n"
		"  pull [<package> [-p <provider>] [-v <version>] ...]    update and install packages\n"
		"  drop <package> ...                                     uninstall and remove packages\n"
		"  list                                                   display installed packages\n"
		"  help                                                   display this help text and quit\n"
	);
}

void jpw::list() {
	if (!fs::is_directory(pkgs)) return;

	for (auto & entry : fs::directory_iterator(pkgs))
		if (entry.is_directory() && fs::is_regular_file(entry.path() / ".metadata"))
			printf("%s\n", entry.path().filename().c_str());
}
