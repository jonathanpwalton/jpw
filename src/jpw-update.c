#include "jpw-update.h"

void update (int argc, char ** argv) {
  if (argc != 3) {
    fprintf(stderr, "error: command 'update' requires exactly one of the following arguments: {all | available | installed}\n");
    exit(1);
  }

  if (strcmp(argv[2], "all") == 0) {
    fprintf(stderr, "update all not implemented\n");
    exit(1);
  } else if (strcmp(argv[2], "available") == 0) {
    fprintf(stderr, "update available not implemented\n");
    exit(1);
  } else if (strcmp(argv[2], "installed") == 0) {
    fprintf(stderr, "update installed not implemented\n");
    exit(1);
  } else {
    fprintf(stderr, "error: argument '%s' is not valid for command 'update', see '%s help' for more information\n", argv[2], argv[0]);
    exit(1);
  }
}
