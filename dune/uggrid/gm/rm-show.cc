// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
#include "config.h"

#include <cerrno>
#include <cstring>

#include <dune/common/exceptions.hh>

#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/initlow.h>
#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/gm/initgm.h>
#include <dune/uggrid/gm/rm.h>

#include <dune/uggrid/gm/rm-write2file.h>

int main(int argc, char** argv)
{
  NS_PREFIX InitLow();
  NS_PREFIX InitDevices();
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
