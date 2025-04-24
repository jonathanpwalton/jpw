#include "core.h"

int main_pull(char ** argv) {
	require(strcmp(*(argv++), "pull") == 0);

	urlopen("https://github.com/jonathanpwalton/packages/raw/main/wget", stdout, true);

	todo();
	return 0;
}
