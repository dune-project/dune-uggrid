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

static ComProcPtr realGather;
static ComProcPtr realScatter;

static int realGatherWrapper(DDD::DDDContext&, DDD_OBJ obj, void* data)
{
  return realGather(obj, data);
}

static int realScatterWrapper(DDD::DDDContext&, DDD_OBJ obj, void* data)
{
  return realGather(obj, data);
}

void DDD_IFOneway(DDD_IF interface, DDD_IF_DIR direction, std::size_t size, ComProcPtr gather, ComProcPtr scatter)
{
  realGather = gather;
  realScatter = scatter;

  DDD_IFOneway(globalDDDContext(), interface, direction, size, realGatherWrapper, realScatterWrapper);
}

int* DDD_InfoProcList(DDD_HDR hdr)
{
  return DDD_InfoProcList(globalDDDContext(), hdr);
}

END_UGDIM_NAMESPACE
