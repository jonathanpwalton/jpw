#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main (int argc, char ** argv) {
	if (setuid(1) != 0) {
		printf("you must be a root user to use this command\n");
		return 1;
	}

	if (argc != 2) {
		printf("usage: %s COMMAND\n", argc >= 1 ? argv[0] : "droproot");
		return 1;
	}

	return system(argv[1]);
}