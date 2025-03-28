#pragma once

#include <string>

namespace curl {

  bool download(std::string const & url, std::string const & dst);

}