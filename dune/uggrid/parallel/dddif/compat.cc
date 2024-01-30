// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
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

  auto& dddctrl = ddd_ctrl(*context);

  ElementIF = dddctrl.ElementIF;
  ElementSymmIF = dddctrl.ElementSymmIF;
  ElementVIF = dddctrl.ElementVIF;
  ElementSymmVIF = dddctrl.ElementSymmVIF;
  ElementVHIF = dddctrl.ElementVHIF;
  ElementSymmVHIF = dddctrl.ElementSymmVHIF;

  BorderNodeIF = dddctrl.BorderNodeIF;
  BorderNodeSymmIF = dddctrl.BorderNodeSymmIF;
  OuterNodeIF = dddctrl.OuterNodeIF;
  NodeVIF = dddctrl.NodeVIF;
  NodeIF = dddctrl.NodeIF;
  NodeAllIF = dddctrl.NodeAllIF;

  BorderVectorIF = dddctrl.BorderVectorIF;
  BorderVectorSymmIF = dddctrl.BorderVectorSymmIF;
  OuterVectorIF = dddctrl.OuterVectorIF;
  OuterVectorSymmIF = dddctrl.OuterVectorSymmIF;
  VectorVIF = dddctrl.VectorVIF;
  VectorVAllIF = dddctrl.VectorVAllIF;
  VectorIF = dddctrl.VectorIF;

  EdgeIF = dddctrl.EdgeIF;
  BorderEdgeSymmIF = dddctrl.BorderEdgeSymmIF;
  EdgeHIF = dddctrl.EdgeHIF;
  EdgeVHIF = dddctrl.EdgeVHIF;
  EdgeSymmVHIF = dddctrl.EdgeSymmVHIF;
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

END_UGDIM_NAMESPACE
