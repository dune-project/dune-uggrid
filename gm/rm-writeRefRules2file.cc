#include "config.h"

#include <cerrno>
#include <cstring>
#include <vector>

#include <dune/common/exceptions.hh>

#include <dune/uggrid/low/namespace.h>

#include <gm/rm-write2file.h>

#include <iostream>
#include <memory>
#include <utility>
int main(int argc, char** argv)
{
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <RefRules.data> <RefRules.cc>\n";
    return 1;
  }

  std::vector<NS_DIM_PREFIX REFRULE> rules;
  std::vector<NS_PREFIX SHORT> patterns;

  // Read rules
  {
    const char* filename = argv[1];
    std::FILE* stream = std::fopen(filename, "r");
    if (!stream)
      DUNE_THROW(Dune::Exception, "Could not open " << filename << " for reading: " << std::strerror(errno));
    readTetrahedronRules(stream, rules, patterns);
    std::fclose(stream);
  }

  // Write rules
  {
    const char* filename = argv[2];
    std::FILE* stream = std::fopen(filename, "w");
    if (!stream)
      DUNE_THROW(Dune::Exception, "Could not open " << filename << " for writing: " << std::strerror(errno));
    Write2File(stream, rules, patterns);
    if (std::fclose(stream))
      DUNE_THROW(Dune::Exception, "E: Closing " << filename << " failed: " << std::strerror(errno));
  }

  return 0;
}
