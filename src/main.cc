#include <stdio.h>
#include "jpw.h"

namespace jpw { queue<string> argv; int indent; Path root, bins, conf, temp, pkgs; }

int main(int cargc, char ** cargv) {
	using namespace jpw;

	for (int i = 1; i < cargc; i++)
		if (cargv[i][0]) argv.push(cargv[i]);

	set_root("/");

	try {
		if (argv.empty() || argv.front() == "help")
			help();
		else if (argv.front() == "list")
			list();
		else if (argv.front() == "pull")
			pull();
		else if (argv.front() == "drop")
			drop();
		else
			throw ProgramError("unknown command, try 'jpw help' for more information");
	} catch (ProgramError & e) {
		fprintf(stderr, "%*s\033[0m\033[1;91merror: \033[0m%s\n", indent, "", e.what());
		return 1;
	}

	return 0;
}

void jpw::set_root(Path const & path) {
	root = path;
	bins = path / "usr/local/bin";
	conf = path / "etc/jpw";
	temp = path / "var/cache/jpw";
	pkgs = path / "var/lib/jpw";
}

void jpw::permit() {
	try {
		for (auto const & path : { root, bins, conf, temp, pkgs }) {
			if (!fs::is_directory(path))
				fs::remove_all(path);
			fs::create_directories(path);
			if (access(path.c_str(), R_OK | W_OK | X_OK) != 0)
				throw fs::filesystem_error("", std::make_error_code(std::errc::operation_not_permitted));
		}
	} catch (fs::filesystem_error & e) {
		if (e.code().value() == (int) std::errc::operation_not_permitted)
			throw ProgramError("you do not have permission to perform this action");
		else
			throw e;
	}
}

void jpw::stage_beg(string const & msg) {
	printf("\033[0m\033[1;97m%*s%s %s\033[0m\n", indent, "", indent == 0 ? "::" : "=>", msg.c_str());
	indent += 3;
}

void jpw::stage_end() {
	indent -= 3;
}

size_t std::filesystem::entry_count(path const & dir) {
	using di = directory_iterator;
	return std::distance(di(dir), di{});
}

