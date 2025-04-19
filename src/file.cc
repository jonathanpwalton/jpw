#include "jpw.h"
#include <string.h>

jpw::File::File(Path const & path, string const & mode) : self(fopen(path.c_str(), mode.c_str())), buffer(nullptr), length(0) {
	if (!self) throw std::runtime_error(strerror(errno));
}

jpw::File::File() : self(open_memstream(&buffer, &length)) {
	if (!self) throw std::runtime_error(strerror(errno));
}

jpw::File::~File() {
	fclose(self);
	if (buffer) free(buffer);
}

void jpw::File::writeln(string const & line) {
	fprintf(self, "%s\n", line.c_str());
}

jpw::vector<jpw::string> jpw::File::readlines() {
	vector<string> lines;

	if (buffer) {
		string line;
		size_t i;

		while (i <= length) {
			if (i == length || buffer[i] == '\n') {
				lines.push_back(line);
				line.clear();
				i++;
			} else {
				line += buffer[i++];
			}
		}
	} else {
		char * line = nullptr;
		size_t i;

		while (getline(&line, &i, self) != -1) {
			lines.push_back(line);
			if (lines.back().ends_with('\n')) lines.back().pop_back();
			free(line);
			line = nullptr;
		}
		free(line);
	}

	return lines;
}

jpw::File::operator FILE * () {
	return self;
}