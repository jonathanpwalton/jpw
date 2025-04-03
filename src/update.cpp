#include "jpw.hpp"

void update::available() {
  error("the 'update' command requires write permission", !permitted());
  chdir(prefix.c_str());
  error("failed to update available packages", !download("https://raw.githubusercontent.com/jonathanpwalton/jpw/main/available.json"));
}

void update::installed() {
  error("the 'update' command requires write permission", !permitted());
}
