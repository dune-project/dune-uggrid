// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      topo.c                                                        */
/*                                                                          */
/* Purpose:   maintains communication structure for data-dependent          */
/*            communication topology                                        */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/30 kb  begin                                            */
/*            95/10/05 kb  added channel disconnection to TopoExit          */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*            system include files                                          */
/*            application include files                                     */
/*                                                                          */
/****************************************************************************/

/* standard C library */
#include <config.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <iomanip>
#include <iostream>
#include <vector>

#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <low/namespace.h>

#include "memmgr.h"

#include "dddi.h"

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

namespace DDD {

/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


/* TODO memory usage is O(P) in current implementation! */

void ddd_TopoInit(DDD::DDDContext& context)
{
  auto& ctx = context.topoContext();
  const auto procs = context.procs();

  /* get one channel pointer for each partner */
  ctx.theTopology.assign(procs, nullptr);

  /* get proc array with maxsize = 2 * number of procs */
  ctx.theProcArray.resize(2 * procs);
}


void ddd_TopoExit(DDD::DDDContext& context)
{
  auto& ctx = context.topoContext();

  ctx.theProcArray.clear();

  /* disconnect channels */
  for (const auto ch : ctx.theTopology)
  {
    if (ch != nullptr)
    {
      DiscASync(context.ppifContext(), ch);
      while (InfoADisc(context.ppifContext(), ch)!=1)
        ;
    }
  }

  ctx.theTopology.clear();
}


/****************************************************************************/


DDD_PROC* DDD_ProcArray(DDD::DDDContext& context)
{
  return context.topoContext().theProcArray.data();
}


RETCODE DDD_GetChannels(DDD::DDDContext& context, int nPartners)
{
  auto& ctx = context.topoContext();
  int i, nConn;

  if (nPartners > 2*(context.procs()-1))
  {
    DDD_PrintError('E', 1520, "topology error in DDD_GetChannels");
    RET_ON_ERROR;
  }

  std::vector<bool> theProcFlags(nPartners);

  nConn = 0;
  for(i=0; i<nPartners; i++)
  {
    if (ctx.theTopology[ctx.theProcArray[i]] == nullptr)
    {
      VChannelPtr vc = ConnASync(context.ppifContext(), ctx.theProcArray[i], VC_TOPO);

      if (vc==NULL)
      {
        Dune::dwarn << "DDD_GetChannels: can't connect to proc="
                    << ctx.theProcArray[i] << "\n";
        RET_ON_ERROR;
      }

      ctx.theTopology[ctx.theProcArray[i]] = vc;
      nConn++;

      theProcFlags[i] = true;
    }
    else
    {
      theProcFlags[i] = false;
    }
  }


  while (nConn>0)
  {
    for(i=0; i<nPartners; i++)
    {
      if (theProcFlags[i])
      {
        int ret = InfoAConn(context.ppifContext(), ctx.theTopology[ctx.theProcArray[i]]);
        if (ret==-1)
        {
          Dune::dwarn << "DDD_GetChannels: InfoAConn() failed for connect to proc="
                      << ctx.theProcArray[i] << "\n";
          RET_ON_ERROR;
        }


        if (ret==1)
        {
          theProcFlags[i] = false;
          nConn--;
        }
      }
    }
  }
  /* TODO free unused channels? */

  RET_ON_OK;
}


void DDD_DisplayTopo (const DDD::DDDContext& context)
{
  using std::setw;

  std::ostream& out = std::cout;
  const auto& ctx = context.topoContext();
  const auto me = context.me();
  const auto procs = context.procs();

  DDD_SyncAll(context);

  if (me == 0) {
    out << "      ";
    for(int p=0; p < procs; ++p)
      out << setw(2) << p;
    out << std::endl;
  }

  for(int p = 0; p < procs; ++p)
  {
    Synchronize(context.ppifContext());
    if (p == me) {
      out << setw(4) << me << ": ";
      for(int i = 0; i < procs; ++i) {
        if (ctx.theTopology[i]!=NULL)
          out << "<>";
        else {
          if (i==p)
            out << "--";
          else
            out << "  ";
        }
      }
      out << std::endl;
    }
  }

  DDD_SyncAll(context);
}


/****************************************************************************/

} /* namespace DDD */
