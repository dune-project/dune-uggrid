// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      notify.c                                                      */
/*                                                                          */
/* Purpose:   notifies destinations for communication with globally         */
/*            unknown topology                                              */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/01/17 kb  begin                                            */
/*            95/04/06 kb  added SpreadNotify                               */
/*            96/07/12 kb  united xxxNotify functions to one DDD_Notify()   */
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

#include <algorithm>
#include <new>
#include <tuple>

#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>
#include "notify.h"

#include <dune/uggrid/parallel/ddd/dddcontext.hh>
#include <dune/uggrid/parallel/ppif/ppifcontext.hh>

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

#define DebugNotify   10  /* 0 is all, 10 is off */

#define PROC_INVALID_TEMP   -1

/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/

namespace DDD {

using namespace DDD::Basic;

static int MAX_INFOS(int procs)
{
  return procs * std::max(1+procs, 10);
}

void NotifyInit(DDD::DDDContext& context)
{
  auto& ctx = context.notifyContext();
  const auto procs = context.procs();

  /* allocate memory */
  ctx.theRouting.resize(procs);

  ctx.maxInfos = MAX_INFOS(procs);     /* TODO maximum value, just for testing */

  /* init local array for all Info records */
  ctx.allInfoBuffer.resize(ctx.maxInfos);

  /* allocate array of NOTIFY_DESCs */
  ctx.theDescs.resize(procs-1);
}


void NotifyExit(DDD::DDDContext& context)
{
  auto& ctx = context.notifyContext();

  /* free memory */
  ctx.theRouting.clear();
  ctx.allInfoBuffer.clear();
  ctx.theDescs.clear();
}


/****************************************************************************/

static bool sort_XferInfos(const NOTIFY_INFO& a, const NOTIFY_INFO& b)
{
  return std::tie(a.to, a.from) < std::tie(b.to, b.from);
}

static bool sort_XferFlags(const NOTIFY_INFO& a, const NOTIFY_INFO& b)
{
  return a.flag < b.flag;
}

static
NOTIFY_INFO *NotifyPrepare (DDD::DDDContext& context)
{
  auto& ctx = context.notifyContext();
  const auto& me = context.me();

#if     DebugNotify<=4
  printf("%4d:    NotifyPrepare\n", me);
  fflush(stdout);
#endif

  /* init local array for all Info records */
  NOTIFY_INFO* allInfos = ctx.allInfoBuffer.data();


  /* init local routing array */
  ctx.theRouting[me] = -1;


  /* dummy Info if there is no message to be send */
  allInfos[0].from = me;
  allInfos[0].to   = PROC_INVALID_TEMP;
  allInfos[0].size = 0;
  allInfos[0].flag = NotifyTypes::DUMMY;
  ctx.lastInfo = 1;

  return allInfos;
}



/****************************************************************************/

/*
        If the parameter 'exception' is !=0, this processor invokes a
        global exception, which will cause all processors to abort this
        notify procedure and return the exception code with flipped sign.
        If more processors issue exception codes, the maximum will be
        communicated.
 */

static
int NotifyTwoWave(DDD::DDDContext& context, NOTIFY_INFO *allInfos, int lastInfo, int exception)
{
  auto& ctx = context.notifyContext();
  const auto& me = context.me();
  const auto& degree = context.ppifContext().degree();

  NOTIFY_INFO  *newInfos;
  int l, i, j, n, unknownInfos, myInfos;
  int local_exception = exception;

#if     DebugNotify<=4
  printf("%4d:    NotifyTwoWave, lastInfo=%d\n", me, lastInfo);
  fflush(stdout);
#endif

  /* BOTTOM->TOP WAVE */
  /* get local Info lists from downtree */
  for(l=degree-1; l>=0; l--)
  {
    GetConcentrate(context.ppifContext(), l, &n, sizeof(int));

    if (n<0)
    {
      /* exception from downtree, propagate */
      if (-n > local_exception)
        local_exception = -n;
    }

    if (lastInfo+n >= ctx.maxInfos) {
      DDD_PrintError('E', 6321, "msg-info array overflow in NotifyTwoWave");
      local_exception = EXCEPTION_NOTIFY;

      /* receive data, but put it onto dummy position */
      GetConcentrate(context.ppifContext(), l, allInfos, n*sizeof(NOTIFY_INFO));
    }
    else
    {
      if (n>0)
        GetConcentrate(context.ppifContext(), l, &(allInfos[lastInfo]), n*sizeof(NOTIFY_INFO));
    }

    /* construct routing table */
    for(i=0; i<n; i++)
      ctx.theRouting[allInfos[lastInfo+i].from] = l;

    if (n>0)
      lastInfo += n;
  }


  if (! local_exception)
  {
    /* determine target direction in tree */
    /* TODO: eventually extra solution for root node!
                     (it knows all flags are MYSELF or KNOWN!)  */
    std::sort(allInfos, allInfos + lastInfo, sort_XferInfos);
    i = j = 0;
    unknownInfos = lastInfo;
    myInfos = 0;
    while (i<lastInfo && allInfos[j].to==PROC_INVALID_TEMP)
    {
      if (allInfos[j].from==allInfos[i].to)
      {
        allInfos[i].flag = (allInfos[i].to==me) ? NotifyTypes::MYSELF : NotifyTypes::KNOWN;
        unknownInfos--;
        if (allInfos[i].to==me)
          myInfos++;
        i++;
      } else {
        if (allInfos[j].from<allInfos[i].to)
          j++;
        else
          i++;
      }
    }
    std::sort(allInfos, allInfos + lastInfo, sort_XferFlags);

    /* send local Info list uptree, but only unknown Infos */
    newInfos = &allInfos[lastInfo-unknownInfos];
    Concentrate(context.ppifContext(), &unknownInfos, sizeof(int));
    Concentrate(context.ppifContext(), newInfos, unknownInfos*sizeof(NOTIFY_INFO));
    lastInfo -= unknownInfos;

                #if     DebugNotify<=1
    for(i=0; i<unknownInfos; i++)
    {
      printf("%4d:    NotifyTwoWave, "
             "send uptree unknown %d/%d (%d|%d;%d)\n",
             me, i, unknownInfos,
             newInfos[i].to, newInfos[i].from, newInfos[i].size);
    }
                #endif

  }
  else
  {
    /* we have an exception somewhere in the processor tree */
    /* propagate it */
    int neg_exception = -local_exception;
    Concentrate(context.ppifContext(), &neg_exception, sizeof(int));
    /* don't need to send data now */
  }

#if     DebugNotify<=3
  printf("%4d:    NotifyTwoWave, wave 1 ready\n", me);
  fflush(stdout);
#endif



  /* TOP->BOTTOM WAVE */

  /* get Infos local to my subtree from uptree */
  unknownInfos = 0;
  GetSpread(context.ppifContext(), &unknownInfos, sizeof(int));
  if (unknownInfos<0)
  {
    /* exception from downtree, propagate */
    if (-unknownInfos > local_exception)
      local_exception = -unknownInfos;
  }

  if (unknownInfos>0)
  {
    GetSpread(context.ppifContext(), newInfos, unknownInfos*sizeof(NOTIFY_INFO));
    lastInfo += unknownInfos;
  }

  if (! local_exception)
  {
    /* sort Infos according to routing */
    std::sort(
      allInfos, allInfos + lastInfo,
      [&ctx](const NOTIFY_INFO& a, const NOTIFY_INFO& b) {
        return ctx.theRouting[a.to] < ctx.theRouting[b.to];
      });

                #if     DebugNotify<=1
    for(i=0; i<lastInfo; i++)
    {
      printf("%4d:    NotifyTwoWave, "
             "sorted for routing  %d/%d (%d|%d;%d)\n",
             me, i, lastInfo,
             allInfos[i].to, allInfos[i].from, allInfos[i].size);
    }
                #endif

    /* send relevant Infos downtree */
    i = 0;
    unknownInfos = lastInfo;
    while ((i<unknownInfos)&&(allInfos[i].to==me)) i++;
    lastInfo = i;
    for(l=0; l<degree; l++)
    {
      j = i;
      while ((i<unknownInfos)&&(ctx.theRouting[allInfos[i].to]==l)) i++;
      j = i-j;

      Spread(context.ppifContext(), l, &j, sizeof(int));
      if (j>0)
        Spread(context.ppifContext(), l, &allInfos[i-j], j*sizeof(NOTIFY_INFO));
    }


    /* reuse theDescs-array for registering messages to be received */
    for(i=0; i<lastInfo; i++)
    {
      ctx.theDescs[i].proc = allInfos[i].from;
      ctx.theDescs[i].size = allInfos[i].size;
    }

                #if     DebugNotify<=3
    printf("%4d:    NotifyTwoWave, "
           "wave 2 ready, nRecv=%d\n", me, lastInfo);
    fflush(stdout);
                #endif
  }
  else
  {
    /* we received an exception from uptree, propagate it */
    for(l=0; l<degree; l++)
    {
      int neg_exception = -local_exception;
      Spread(context.ppifContext(), l, &neg_exception, sizeof(int));
      /* dont send any data */
    }

                #if     DebugNotify<=3
    printf("%4d:    NotifyTwoWave, "
           "wave 2 ready, Exception=%d\n", me, local_exception);
    fflush(stdout);
                #endif

    return(-local_exception);
  }

  return(lastInfo);
}



/****************************************************************************/


NOTIFY_DESC *DDD_NotifyBegin(DDD::DDDContext& context, int n)
{
  auto& ctx = context.notifyContext();

  ctx.nSendDescs = n;

  /* allocation of theDescs is done in NotifyInit() */

  if (n > context.procs()-1)
  {
    DDD_PrintError('E', 6340,
                   "more send-messages than other processors in DDD_NotifyBegin");
    return nullptr;
  }

  return ctx.theDescs.data();
}


void DDD_NotifyEnd(DDD::DDDContext&)
{
  /* free'ing of theDescs is done in NotifyExit() */
}


int DDD_Notify(DDD::DDDContext& context)
{
  auto& ctx = context.notifyContext();
  int i, nRecvMsgs;

  const auto me = context.me();
  const auto procs = context.procs();

  /* get storage for local info list */
  NOTIFY_INFO* allInfos = NotifyPrepare(context);
  if (allInfos == nullptr) return(ERROR);

  if (ctx.nSendDescs<0)
  {
    /* this processor is trying to send a global notification
       message. this is necessary for communicating fatal error
       conditions to all other processors. */

    Dune::dwarn
      << "DDD_Notify: proc " << me
      << " is sending global exception #" << (-ctx.nSendDescs) << "\n";

    /* notify partners */
    nRecvMsgs = NotifyTwoWave(context, allInfos, ctx.lastInfo, -ctx.nSendDescs);
  }
  else
  {
    /* convert message list to local Info list */
    for(i=0; i<ctx.nSendDescs; i++)
    {
                        #if     DebugNotify<=4
      printf("%4d:    Notify send msg #%02d to %3d size=%d\n", me,
             ctx.lastInfo, ctx.theDescs[i].proc, ctx.theDescs[i].size);
                        #endif

      if (ctx.theDescs[i].proc==me) {
        Dune::dwarn << "DDD_Notify: proc " << me
                    << " is trying to send message to itself\n";
        return(ERROR);
      }
      if (ctx.theDescs[i].proc>=procs) {
        Dune::dwarn
          << "DDD_Notify: proc " << me << " is trying to send message to proc "
          << ctx.theDescs[i].proc << "\n";
        return(ERROR);
      }

      allInfos[ctx.lastInfo].from = me;
      allInfos[ctx.lastInfo].to   = ctx.theDescs[i].proc;
      allInfos[ctx.lastInfo].size = ctx.theDescs[i].size;
      allInfos[ctx.lastInfo].flag = NotifyTypes::UNKNOWN;
      ctx.lastInfo++;
    }

    /* notify partners */
    nRecvMsgs = NotifyTwoWave(context, allInfos, ctx.lastInfo, 0);
  }


#       if      DebugNotify<=4
  for(i=0; i<nRecvMsgs; i++)
  {
    printf("%4d:    Notify recv msg #%02d from %3d size=%d\n", me,
           ctx.lastInfo, ctx.theDescs[i].proc, ctx.theDescs[i].size);
  }
#       endif


  return(nRecvMsgs);
}

/****************************************************************************/

} /* namespace DDD */
