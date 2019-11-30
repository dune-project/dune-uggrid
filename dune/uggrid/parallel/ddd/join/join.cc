// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      join.c                                                        */
/*                                                                          */
/* Purpose:   main module for object join environment                       */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: birken@ica3.uni-stuttgart.de                           */
/*            phone: 0049-(0)711-685-7007                                   */
/*            fax  : 0049-(0)711-685-7000                                   */
/*                                                                          */
/* History:   980122 kb  begin                                              */
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

#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>

USING_UG_NAMESPACE

  START_UGDIM_NAMESPACE

/*
        NOTE: all container-classes from ooppcc.h are implemented in this
              source file by setting the following define.
 */
#define ContainerImplementation
#define _CHECKALLOC(ptr)   assert(ptr!=NULL)


END_UGDIM_NAMESPACE

#include "join.h"

START_UGDIM_NAMESPACE


/****************************************************************************/
/*                                                                          */
/* class member function implementations                                    */
/*                                                                          */
/****************************************************************************/

#define ClassName JIJoin

/*
        compare-method in order to eliminate double JIJoin-items.

        the items are sorted according to key (dest,remote_gid),
        all in ascending order.
 */
int Method(Compare) (ClassPtr item1, ClassPtr item2, const DDD::DDDContext*)
{
  if (item1->dest < item2->dest) return(-1);
  if (item1->dest > item2->dest) return(1);

  if (item1->new_gid < item2->new_gid) return(-1);
  if (item1->new_gid > item2->new_gid) return(1);


  /* items have equal gid and dest, so they are considered as equal. */
  return(0);
}


void Method(Print) (ParamThis _PRINTPARAMS)
{
  fprintf(fp, "JIJoin local_gid=" OBJ_GID_FMT " dest=%d new_gid=" DDD_GID_FMT "\n",
          OBJ_GID(This->hdr), This->dest, This->new_gid);
}

#undef ClassName




#define ClassName JIAddCpl

/*
        compare-method in order to eliminate double JIAddCpl-items.

        the items are sorted according to key (dest,gid),
        all in ascending order.
 */
int Method(Compare) (ClassPtr item1, ClassPtr item2, const DDD::DDDContext*)
{
  if (item1->dest < item2->dest) return(-1);
  if (item1->dest > item2->dest) return(1);

  if (item1->te.gid < item2->te.gid) return(-1);
  if (item1->te.gid > item2->te.gid) return(1);

  if (item1->te.proc < item2->te.proc) return(-1);
  if (item1->te.proc > item2->te.proc) return(1);


  /* items have equal gid and dest, so they are considered as equal. */
  return(0);
}


void Method(Print) (ParamThis _PRINTPARAMS)
{
  fprintf(fp, "JIAddCpl gid=" DDD_GID_FMT " dest=%d proc=%d prio=%d\n",
          This->te.gid, This->dest, This->te.proc, This->te.prio);
}

#undef ClassName




/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


/*
        management functions for JoinMode.

        these functions control the mode the join-module is
        currently in. this is used for error detection, but
        also for correct detection of coupling inconsistencies
        and recovery.
 */

const char *JoinModeName(JoinMode mode)
{
  switch(mode)
  {
  case JoinMode::JMODE_IDLE : return "idle-mode";
  case JoinMode::JMODE_CMDS : return "commands-mode";
  case JoinMode::JMODE_BUSY : return "busy-mode";
  }
  return "unknown-mode";
}


static void JoinSetMode (DDD::DDDContext& context, JoinMode mode)
{
  auto& ctx = context.joinContext();

  ctx.joinMode = mode;

#       if DebugJoin<=8
  Dune::dinfo << "JoinMode=" << JoinModeName(ctx.joinMode) << "\n";
#       endif
}


static JoinMode JoinSuccMode (JoinMode mode)
{
  switch(mode)
  {
  case JoinMode::JMODE_IDLE : return JoinMode::JMODE_CMDS;
  case JoinMode::JMODE_CMDS : return JoinMode::JMODE_BUSY;
  case JoinMode::JMODE_BUSY : return JoinMode::JMODE_IDLE;
  }
  DUNE_THROW(Dune::InvalidStateException, "invalid JoinMode");
}


bool ddd_JoinActive(const DDD::DDDContext& context)
{
  return context.joinContext().joinMode != JoinMode::JMODE_IDLE;
}


bool JoinStepMode(DDD::DDDContext& context, JoinMode old)
{
  auto& ctx = context.joinContext();
  if (ctx.joinMode != old)
  {
    Dune::dwarn
      << "wrong join-mode (currently in " << JoinModeName(ctx.joinMode)
      << ", expected " << JoinModeName(old) << ")\n";
    return false;
  }

  JoinSetMode(context, JoinSuccMode(ctx.joinMode));
  return true;
}


/****************************************************************************/


void ddd_JoinInit(DDD::DDDContext& context)
{
  auto& ctx = context.joinContext();

  /* init control structures for JoinInfo-items in messages */
  ctx.setJIJoin    = reinterpret_cast<DDD::Join::JIJoinSet*>(New_JIJoinSet());
  ctx.setJIAddCpl2 = reinterpret_cast<DDD::Join::JIAddCplSet*>(New_JIAddCplSet());
  ctx.setJIAddCpl3 = reinterpret_cast<DDD::Join::JIAddCplSet*>(New_JIAddCplSet());

  JoinSetMode(context, JoinMode::JMODE_IDLE);

  ctx.phase1msg_t = LC_NewMsgType(context, "Join1Msg");
  ctx.jointab_id = LC_NewMsgTable("GidTab",
                                  ctx.phase1msg_t, sizeof(TEJoin));

  ctx.phase2msg_t = LC_NewMsgType(context, "Join2Msg");
  ctx.addtab_id = LC_NewMsgTable("AddCplTab",
                                 ctx.phase2msg_t, sizeof(TEAddCpl));

  ctx.phase3msg_t = LC_NewMsgType(context, "Join3Msg");
  ctx.cpltab_id = LC_NewMsgTable("AddCplTab",
                                 ctx.phase3msg_t, sizeof(TEAddCpl));
}


void ddd_JoinExit(DDD::DDDContext& context)
{
  auto& ctx = context.joinContext();

  JIJoinSet_Free(reinterpret_cast<JIJoinSet*>(ctx.setJIJoin));
  JIAddCplSet_Free(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl2));
  JIAddCplSet_Free(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));
}




END_UGDIM_NAMESPACE
