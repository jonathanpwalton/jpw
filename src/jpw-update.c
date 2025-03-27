#include "jpw-update.h"

void update_available();
void update_installed();

void update (int argc, char ** argv) {
  if (argc != 3) {
    fprintf(stderr, "error: command 'update' requires exactly one of the following arguments: {all | available | installed}\n");
    exit(1);
  }

  setupdirs(argv);

  if (access(DBA, F_OK) != 0) {
    FILE * tmp = fopen(DBA, "w");
    
    if (!tmp) {
      fprintf(stderr, "error: command 'update' requires root privilege\n");
      exit(1);
    } else {
      fclose(tmp);
      unlink(DBA);
    }
  } else if (unlink(DBA) != 0) {
    fprintf(stderr, "error: command 'update' requires root privilege\n");
    exit(1);
  }

  if (strcmp(argv[2], "all") == 0) {
    update_available();
    update_installed();
  } else if (strcmp(argv[2], "available") == 0) {
    update_available();
  } else if (strcmp(argv[2], "installed") == 0) {
    update_installed();
  } else {
    fprintf(stderr, "error: argument '%s' is not valid for command 'update', see '%s help' for more information\n", argv[2], argv[0]);
    exit(1);
  }
}

void update_available() {
  char const * const SRC = "https://raw.githubusercontent.com/jonathanpwalton/jpw/main/available.db";
  char const * const DST = "/var/lib/jpw/available";

  if (download(DST, SRC) != 0) {
    fprintf(stderr, "error: failed to retrieve available package database from the server\n");
    exit(1);
  }
}

void update_installed() {
  fprintf(stderr, "update installed not implemented\n");
  exit(1);
}
