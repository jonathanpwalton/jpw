#define _POSIX_C_SOURCE 2
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/wait.h>

int main (int argc, char ** argv) {
	if (setuid(SHRT_MAX) != 0) {
		int succeeded = 0;
		while (errno == EAGAIN) {
			if (setuid(1) == 0) {
				succeeded = 1;
				break;
			}
		}

		if (succeeded == 0) {
			if (errno == EINVAL)
				printf("droproot: error: invalid user ID `%u`\n", SHRT_MAX);
			else if (errno == EPERM)
				printf("droproot: error: you are not permitted to use this command\n");
			return 1;
		}
	}

	if (argc != 2) {
		printf("droproot: usage: %s COMMAND\n", argc >= 1 ? argv[0] : "droproot");
		return 1;
	}

	int status = system(argv[1]);

	if (status == -1) {
		printf("droproot: error: failed to call system(): %s", strerror(errno));
		return 1;
	}

	return WEXITSTATUS(status);
}