#include "jpw.h"

struct version {
  char const * const ver;
  char const * const src;
  char const * const dst;
  char const * const bld;
};

struct version find_available(char const * const package, char const * const version);