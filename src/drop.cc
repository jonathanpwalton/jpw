#include "jpw.h"
#include <unordered_map>

using std::unordered_map;

void jpw::drop() {
	argv.pop();
	permit();

	if (argv.empty())
		throw ProgramError("expected one or more package names, try 'jpw help' for more information");

	while (!argv.empty()) {
		auto & name = argv.back();

		argv.pop();
	}
}