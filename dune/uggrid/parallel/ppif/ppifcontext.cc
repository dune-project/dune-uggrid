#include "config.h"

#include "ppifcontext.hh"

namespace PPIF {

#if ModelP
PPIFContext
::PPIFContext()
  : PPIFContext(MPI_COMM_WORLD)
{
  /* Nothing. */
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
}
#endif

} /* namespace PPIF */
