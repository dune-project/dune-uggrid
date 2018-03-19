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

#include <vector>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include "memmgr.h"

#include "dddi.h"

#include "namespace.h"

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

START_UGDIM_NAMESPACE

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
  ctx.theTopology = (VChannelPtr *) AllocFix(procs*sizeof(VChannelPtr));
  if (ctx.theTopology == nullptr)
  {
    DDD_PrintError('E', 1500, STR_NOMEM " in TopoInit");
    return;
  }

  /* initialize channel topology */
  for(int i=0; i<procs; i++)
    ctx.theTopology[i] = nullptr;


  /* get proc array with maxsize = 2 * number of procs */
  ctx.theProcArray = (DDD_PROC *) AllocFix(2 * procs*sizeof(DDD_PROC));
  if (ctx.theProcArray == nullptr)
  {
    DDD_PrintError('E', 1510, STR_NOMEM " in TopoInit");
    return;
  }
}


void ddd_TopoExit(DDD::DDDContext& context)
{
  auto& ctx = context.topoContext();
  const auto procs = context.procs();

  FreeFix(ctx.theProcArray);

  /* disconnect channels */
  for(int i=0; i<procs; i++)
  {
    if (ctx.theTopology[i]!=NULL)
    {
      DiscASync(context.ppifContext(), ctx.theTopology[i]);
      while (InfoADisc(context.ppifContext(), ctx.theTopology[i])!=1)
        ;
    }
  }

  FreeFix(ctx.theTopology);
}


/****************************************************************************/


DDD_PROC* DDD_ProcArray(DDD::DDDContext& context)
{
  return context.topoContext().theProcArray;
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
        sprintf(cBuffer,
                "can't connect to proc=%d in DDD_GetChannels",
                ctx.theProcArray[i]);
        DDD_PrintError('E', 1521, cBuffer);
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
          sprintf(cBuffer,
                  "PPIF's InfoAConn() failed for connect to proc=%d"
                  " in DDD_GetChannels",
                  ctx.theProcArray[i]);
          DDD_PrintError('E', 1530, cBuffer);
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
  int p, i;
  char buf[20];

  const auto& ctx = context.topoContext();
  const auto me = context.me();
  const auto procs = context.procs();

  DDD_SyncAll(context);

  if (me==0)
  {
    sprintf(cBuffer, "      ");
    for(p=0; p<procs; p++)
    {
      sprintf(buf, "%2d", p);
      strcat(cBuffer, buf);
    }
    strcat(cBuffer,"\n");
    DDD_PrintLine(cBuffer); fflush(stdout);
  }

  for(p=0; p<procs; p++)
  {
    Synchronize(context.ppifContext());
    if (p==me)
    {
      sprintf(cBuffer, "%4d: ", me);
      for(i=0; i<procs; i++)
      {
        if (ctx.theTopology[i]!=NULL)
        {
          strcat(cBuffer,"<>");
        }
        else
        {
          if (i==p)
            strcat(cBuffer,"--");
          else
            strcat(cBuffer,"  ");
        }
      }
      strcat(cBuffer,"\n");
      DDD_PrintLine(cBuffer);
      DDD_Flush();
    }
  }

  DDD_SyncAll(context);
}


/****************************************************************************/

END_UGDIM_NAMESPACE
