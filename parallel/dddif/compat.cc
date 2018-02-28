#include <config.h>

#include "parallel.h"

#include <cassert>

START_UGDIM_NAMESPACE

std::shared_ptr<DDD::DDDContext> globalDDDContext_ = nullptr;

DDD::DDDContext& globalDDDContext()
{
  assert(globalDDDContext_);
  return *globalDDDContext_;
}

void globalDDDContext(const std::shared_ptr<DDD::DDDContext>& context)
{
  globalDDDContext_ = context;
}

void globalDDDContext(std::nullptr_t)
{
  globalDDDContext_ = nullptr;
}

void DDD_IFOneway(DDD_IF interface, DDD_IF_DIR direction, std::size_t size, ComProcPtr gather, ComProcPtr scatter)
{
  DDD_IFOneway(globalDDDContext(), interface, direction, size, gather, scatter);
}

END_UGDIM_NAMESPACE
