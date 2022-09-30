// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
#include "config.h"

#include "ppifcontext.hh"

namespace PPIF {

#if ModelP
PPIFContext
::PPIFContext()
  : PPIFContext(MPI_COMM_WORLD)
{
  /* Nothing */
}
#else
PPIFContext
::PPIFContext()
{
  /* Nothing */
}
#endif

PPIFContext
::~PPIFContext()
{
#if ModelP
  ExitPPIF(*this);

  int finalized;
  MPI_Finalized(&finalized);

  if (not finalized)
    MPI_Comm_free(&comm_);
#endif
}

#if ModelP
PPIFContext
::PPIFContext(const MPI_Comm& comm)
{
  MPI_Comm_dup(comm, &comm_);
  MPI_Comm_rank(comm_, &me_);
  MPI_Comm_size(comm_, &procs_);

  InitPPIF(*this);
}
#endif

} /* namespace PPIF */
