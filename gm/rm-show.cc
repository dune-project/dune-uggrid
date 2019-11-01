#include "config.h"

#include <cerrno>
#include <cstring>

#include <dune/common/exceptions.hh>

#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/initlow.h>
#include "dev/ugdevices.h"
#include "gm/initgm.h"
#include "gm/rm.h"

#include "gm/rm-write2file.h"

int main(int argc, char** argv)
{
  UG::InitLow();
  UG::InitDevices();
  NS_DIM_PREFIX InitGm();

  std::vector<NS_DIM_PREFIX REFRULE> rules;
  std::vector<NS_PREFIX SHORT> patterns;

  // use an arbitrary additional argument to use the rules from "lib/ugdata/RefRules.data"
  if( argc > 1 )
  {
    // read the file "lib/ugdata/RefRules.data" and use those rules
    std::FILE* stream = std::fopen(argv[1], "r");
    if (!stream)
      DUNE_THROW(Dune::Exception, "Could not open file " << argv[1] << ": " << std::strerror(errno));
    NS_DIM_PREFIX readTetrahedronRules(stream, rules, patterns);
    std::fclose(stream);

    NS_DIM_PREFIX RefRules[NS_DIM_PREFIX TETRAHEDRON] = rules.data();
    NS_DIM_PREFIX MaxRules[NS_DIM_PREFIX TETRAHEDRON] = rules.size();
  }

  // write rules
  for(int i=0; i<NS_DIM_PREFIX MaxRules[NS_DIM_PREFIX TETRAHEDRON]; i++)
  {
    NS_DIM_PREFIX ShowRefRuleX(NS_DIM_PREFIX TETRAHEDRON, i, std::printf);
  }

  return 0;
}
