#include "core.h"

#define X_OPEN_SOURCE 500
#include <ftw.h>
#include <unistd.h>

jpw::Maybe<jpw::str> jpw::IO::readline() {
	char * line = nullptr; size_t size = 0;
	if (getline(&line, &size, file) >= 0) {
		str s = line;
		free(line);
		while (s.back() == '\n')
			s.pop_back();
		return s;
	}
	return false;
}

jpw::list<jpw::str> jpw::IO::readlines() {
	list<str> lines;
	Maybe<str> line;
	while ((line = readline())) 
		lines.push_back(line);
	return lines;
}

jpw::Maybe<jpw::str> jpw::BytesIO::readline() {
	if (offset >= len(buffer)) return false;
	str line;
	while (true) {
		if (offset >= len(buffer) || buffer[offset] == '\0' || buffer[offset] == '\n') {
			offset += offset < len(buffer) && buffer[offset] == '\n' ? 1 : 0;
			return line;
		}
		line += buffer[offset++];
	}
	return false;
}

static uid_t CURRENT_CHOWN_UID;
static int chownf(char const * fpath, struct stat const * sb, int typeflag, struct FTW * ftwbuf) {
	return ::chown(fpath, CURRENT_CHOWN_UID, CURRENT_CHOWN_UID);
}

bool jpw::fs::chown(Path const &path, uid_t uid) {
	CURRENT_CHOWN_UID = uid;
	return nftw(path.c_str(), chownf, 10, FTW_DEPTH | FTW_MOUNT | FTW_PHYS) == 0;
}

bool jpw::pipe(str const &command, list<str> const & env) {
	log(command);

	str cmd; for (const auto & e : env) cmd += e + "; ";
	cmd += command + " 2>&1";

	FILE * p = popen(cmd.c_str(), "r");
	if (!p) throw std::runtime_error("failed to open pipe " + command);

	char * line = nullptr;
	size_t size, length;

	while (getline(&line, &size, p) != -1) {
		log(line, "");
		free(line);
		line = nullptr;
	}
	free(line);

	return WEXITSTATUS(pclose(p)) == 0;
}
