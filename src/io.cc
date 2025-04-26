#include "core.h"

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
