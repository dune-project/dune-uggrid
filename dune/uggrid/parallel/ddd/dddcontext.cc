#include "config.h"

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

namespace DDD {

DDDContext::DDDContext(
  const std::shared_ptr<PPIF::PPIFContext>& ppifContext,
  const std::shared_ptr<void>& data)
  : ppifContext_(ppifContext)
  , data_(data)
{
  /* Nothing. */
}

int
DDDContext::me() const
{
  return ppifContext().me();
}

int
DDDContext::procs() const
{
  return ppifContext().procs();
}

bool
DDDContext::isMaster() const
{
  return ppifContext().isMaster();
}

} /* namespace DDD */
