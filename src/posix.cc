#include "jpw.h"

#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <algorithm>

bool jpw::fork() {
	switch (::fork()) {
		case 0:
			return true;
		case -1:
			throw ProgramError("failed to fork");
		default:
			return false;
	}
}

int jpw::wait() {
	int status;
	if (::wait(&status) == -1) return -1;
	return status;
}

int jpw::pipe(string const & cmd) {
	printf("%*s%s\n", indent, "", cmd.c_str());
	FILE * p = popen(("JPW_BINS='" + bins.string() + "'; " + cmd + " 2>&1").c_str(), "r");
	if (!p) throw std::runtime_error("failed to open pipe " + cmd);

	char * line = nullptr;
	size_t size, length;

	while (getline(&line, &size, p) != -1) {
		length = strlen(line);
		if (!std::all_of(line, line + length, isspace)) printf("%*s%s%c", indent, "", line, line[length - 1] != '\n' ? '\n' : '\0');
		free(line);
		line = nullptr;
	}
	free(line);

	return WEXITSTATUS(pclose(p));
}

void jpw::chown(Path const & path, unsigned int uid) {
	::chown(path.c_str(), uid, uid);
	if (fs::is_directory(path)) {
		for (auto & entry : fs::recursive_directory_iterator(path)) {
			::chown(entry.path().c_str(), uid, uid);
		}
	}
}
