// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      cmds.c                                                        */
/*                                                                          */
/* Purpose:   DDD-commands for Transfer Module                              */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   931130 kb  begin (xfer.c)                                     */
/*            950321 kb  added variable sized objects (XferCopyObjX)        */
/*            950405 kb  V1.3: extracted from xfer.c                        */
/*            960703 kb  split XferInfo-list into ObjXfer and CplXfer    */
/*            960718 kb  introduced lowcomm-layer (sets of messages)        */
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

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>
#include "xfer.h"


USING_UG_NAMESPACE
using namespace PPIF;

  START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/




/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


static int sort_XIDelCmd (const void *e1, const void *e2)
{
  XIDelCmd *item1 = *((XIDelCmd **)e1);
  XIDelCmd *item2 = *((XIDelCmd **)e2);

  /* ascending GID is needed for ExecLocalXIDelCmds */
  if (OBJ_GID(item1->hdr) < OBJ_GID(item2->hdr)) return(-1);
  if (OBJ_GID(item1->hdr) > OBJ_GID(item2->hdr)) return(1);

  return(0);
}


static int sort_XIDelObj (const void *e1, const void *e2)
{
  XIDelObj *item1 = *((XIDelObj **)e1);
  XIDelObj *item2 = *((XIDelObj **)e2);

  /* ascending GID is needed for ExecLocalXIDelObjs */
  if (item1->gid < item2->gid) return(-1);
  if (item1->gid > item2->gid) return(1);

  return(0);
}


static int sort_XINewCpl (const void *e1, const void *e2)
{
  XINewCpl *item1 = *((XINewCpl **)e1);
  XINewCpl *item2 = *((XINewCpl **)e2);

  /* receiving processor */
  if (item1->to < item2->to) return(-1);
  if (item1->to > item2->to) return(1);

  return(0);
}



static int sort_XIOldCpl (const void *e1, const void *e2)
{
  XIOldCpl *item1 = *((XIOldCpl **)e1);
  XIOldCpl *item2 = *((XIOldCpl **)e2);
  DDD_GID gid1, gid2;

  /* receiving processor */
  if (item1->to < item2->to) return(-1);
  if (item1->to > item2->to) return(1);

  /* ascending GID is needed for UnpackOldCplTab on receiver side */
  gid1 = item1->te.gid;
  gid2 = item2->te.gid;
  if (gid1 < gid2) return(-1);
  if (gid1 > gid2) return(1);

  return(0);
}



static int sort_XIDelCpl (const void *e1, const void *e2)
{
  XIDelCpl *item1 = *((XIDelCpl **)e1);
  XIDelCpl *item2 = *((XIDelCpl **)e2);
  DDD_GID gid1, gid2;

  /* receiving processor */
  if (item1->to < item2->to) return(-1);
  if (item1->to > item2->to) return(1);

  /* ascending GID is needed by CplMsgUnpack on receiver side */
  gid1 = item1->te.gid;
  gid2 = item2->te.gid;
  if (gid1 < gid2) return(-1);
  if (gid1 > gid2) return(1);

  return(0);
}


static int sort_XIModCpl (const void *e1, const void *e2)
{
  XIModCpl *item1 = *((XIModCpl **)e1);
  XIModCpl *item2 = *((XIModCpl **)e2);
  DDD_GID gid1, gid2;

  /* receiving processor */
  if (item1->to < item2->to) return(-1);
  if (item1->to > item2->to) return(1);

  /* ascending GID is needed by CplMsgUnpack on receiver side */
  gid1 = item1->te.gid;
  gid2 = item2->te.gid;
  if (gid1 < gid2) return(-1);
  if (gid1 > gid2) return(1);

  /* sorting according to priority is not necessary anymore,
     equal items with different priorities will be sorted
     out according to PriorityMerge(). KB 970129
     if (item1->te.prio < item2->te.prio) return(-1);
     if (item1->te.prio > item2->te.prio) return(1);
   */

  return(0);
}


static int sort_XIAddCpl (const void *e1, const void *e2)
{
  XIAddCpl *item1 = *((XIAddCpl **)e1);
  XIAddCpl *item2 = *((XIAddCpl **)e2);
  DDD_GID gid1, gid2;

  /* receiving processor */
  if (item1->to < item2->to) return(-1);
  if (item1->to > item2->to) return(1);

  /* ascending GID is needed by CplMsgUnpack on receiver side */
  gid1 = item1->te.gid;
  gid2 = item2->te.gid;
  if (gid1 < gid2) return(-1);
  if (gid1 > gid2) return(1);

  return(0);
}



/****************************************************************************/



/*
        eliminate double XIDelCmd-items.

        the items have been sorted according to key
    (gid), all in ascending order.
        if gid (i.e., hdr) is equal,
        the item is skipped.
        this implements rule XFER-D1.

        the number of valid items is returned.
 */
static int unify_XIDelCmd (const DDD::DDDContext&, XIDelCmd **i1, XIDelCmd **i2)
{
  return ((*i1)->hdr != (*i2)->hdr);
}



/*
        eliminate double XIModCpl-items.
        merge priorities from similar XIModCpl-items.

        the items have been sorted according to key (to,gid),
        all in ascending order. if to or gid are different, then
        at least the first item is relevant. if both are equal,
        we merge priorities and get a new priority together with
        the information whether first item wins over second.
        if first item wins, it is switched into second position and
        the second item (now on first position) is rejected.
        if second item wins, first item is rejected.
        in both cases, we use the new priority for next comparison.
 */
static int unify_XIModCpl (const DDD::DDDContext& context, XIModCpl **i1p, XIModCpl **i2p)
{
  XIModCpl *i1 = *i1p, *i2 = *i2p;
  DDD_PRIO newprio;
  int ret;

  /* if items are different in gid or dest, take first item */
  if ((i1->to != i2->to) || (i1->te.gid != i2->te.gid))
    return true;

  /* items have equal to and gid, we must check priority */
  ret = PriorityMerge(&context.typeDefs()[i1->typ],
                      i1->te.prio, i2->te.prio, &newprio);

  if (ret==PRIO_FIRST || ret==PRIO_UNKNOWN)
  {
    /* i1 is winner, take it, switch it into second position,
       signal rejection of i2 (now on first position).
       use new priority */

    i1->te.prio = newprio;
    *i1p = i2; *i2p = i1;              /* switch pointers */
  }
  else
  {
    /* i1 lost, i2 is winner. throw away i1, but
       use new priority for next comparison */
    i2->te.prio = newprio;
  }

  return false;
}



/* TODO remove this */
void GetSizesXIAddData (const DDD::DDDContext& context, int *, int *, size_t *, size_t *);


/*
        compute and display memory resources used
 */
static void DisplayMemResources(const DDD::DDDContext& context)
{
  const auto& ctx = context.xferContext();

  int nSegms=0, nItems=0, nNodes=0;
  size_t memAllocated=0, memUsed=0;

  GetSizesXIAddData(context, &nSegms, &nItems, &memAllocated, &memUsed);
  if (nSegms>0)
    printf("XferEnd, XIAddData segms=%d items=%d allocated=%ld used=%ld\n",
           nSegms, nItems, (long)memAllocated, (long)memUsed);


  XICopyObjSet_GetResources(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj),
                            &nSegms, &nItems, &nNodes, &memAllocated, &memUsed);
  if (nSegms>0) {
    printf("XferEnd, XICopyObj "
           "segms=%d items=%d nodes=%d allocated=%ld used=%ld\n",
           nSegms, nItems, nNodes, (long)memAllocated, (long)memUsed);
  }


        #ifdef XICOPYOBJ_DETAILED_RESOURCES
  /* this is a different version, split up into BTree and SegmList */
  XICopyObjSegmList_GetResources(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj)->list,
                                 &nSegms, &nItems, &memAllocated, &memUsed);
  if (nSegms>0)
    printf("XferEnd, XICopyObj segms=%d items=%d allocated=%ld used=%ld\n",
           nSegms, nItems, (long)memAllocated, (long)memUsed);

  XICopyObjBTree_GetResources(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj)->tree,
                              &nNodes, &nItems, &memAllocated, &memUsed);
  if (nItems>0)
    printf("XferEnd, XICopyObj nodes=%d items=%d allocated=%ld used=%ld\n",
           nNodes, nItems, (long)memAllocated, (long)memUsed);
        #endif


  XISetPrioSet_GetResources(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio),
                            &nSegms, &nItems, &nNodes, &memAllocated, &memUsed);
  if (nSegms>0) {
    printf("XferEnd, XISetPrio "
           "segms=%d items=%d nodes=%d allocated=%ld used=%ld\n",
           nSegms, nItems, nNodes, (long)memAllocated, (long)memUsed);
  }


#define SLL_GET_SIZES(T) do {                                           \
    GetSizes##T(context, &nSegms, &nItems, &memAllocated, &memUsed);    \
    if (nSegms>0)                                                       \
      printf("XferEnd, " #T "  segms=%d items=%d allocated=%ld used=%ld\n", \
             nSegms, nItems, (long)memAllocated, (long)memUsed);    \
  } while(false)
  SLL_GET_SIZES(XIDelCmd);
  SLL_GET_SIZES(XIDelObj);
  SLL_GET_SIZES(XINewCpl);
  SLL_GET_SIZES(XIOldCpl);
  SLL_GET_SIZES(XIDelCpl);
  SLL_GET_SIZES(XIModCpl);
  SLL_GET_SIZES(XIAddCpl);
#undef SLL_GET_SIZES
}


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferEnd                                                   */
/*                                                                          */
/****************************************************************************/

/**
        End of transfer phase.
        This function starts the object transfer process. After a call to
        this function (on all processors) all {\bf Transfer}-commands since
        the last call to \funk{XferBegin} are executed. This involves
        a set of local communications between the processors.
 */

DDD_RET DDD_XferEnd(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();
  const auto& me = context.me();

  DDD_RET ret_code              = DDD_RET_OK;
  XICopyObj   **arrayNewOwners      = NULL;
  int nNewOwners;
  XIDelCmd    **arrayXIDelCmd       = NULL;
  int remXIDelCmd, prunedXIDelCmd;
  XIDelObj    **arrayXIDelObj       = NULL;
  std::vector<XISetPrio*> arrayXISetPrio;
  XINewCpl    **arrayXINewCpl       = NULL;
  XIOldCpl    **arrayXIOldCpl       = NULL;
  XIDelCpl    **arrayXIDelCpl       = NULL;
  int remXIDelCpl;
  XIModCpl    **arrayXIModCpl       = NULL;
  int remXIModCpl;
  XIAddCpl    **arrayXIAddCpl       = NULL;
  int obsolete, nRecvMsgs;
  XFERMSG     *sendMsgs=NULL, *sm=NULL;
  LC_MSGHANDLE *recvMsgs            = NULL;
  std::vector<DDD_HDR> localCplObjs;
  size_t sendMem=0, recvMem=0;
  int DelCmds_were_pruned;

  STAT_SET_MODULE(DDD_MODULE_XFER);
  STAT_ZEROALL;

  const auto procs = context.procs();
  const auto& nCpls = context.couplingContext().nCpls;

  /* step mode and check whether call to XferEnd is valid */
  if (!XferStepMode(context, DDD::Xfer::XferMode::XMODE_CMDS))
    DUNE_THROW(Dune::Exception, "DDD_XferEnd() aborted");


  /*
          PREPARATION PHASE
   */
  STAT_RESET;
  /* get sorted array of XICopyObj-items */
  std::vector<XICopyObj*> arrayXICopyObj = XICopyObjSet_GetArray(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj));
  obsolete = XICopyObjSet_GetNDiscarded(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj));

  /* debugging output, write all XICopyObjs to file
     if (XICopyObjSet_GetNItems(ctx.setXICopyObj)>0)
     {
          FILE *ff = fopen("xfer.dump","w");
          XICopyObjSet_Print(ctx.setXICopyObj, 2, ff);
          fclose(ff);
     }
   */


  /*
          (OPTIONAL) COMMUNICATION PHASE 0
   */
  if (DDD_GetOption(context, OPT_XFER_PRUNE_DELETE)==OPT_ON)
  {
    /*
            for each XferDelete-Cmd: if there exists at least
                    one XferCopy-cmd with destination=me, then the
                    XferDelete-Cmd is discarded.

            NOTE: the priorities behave like in the specification,
                    i.e., incoming objects with lower priority than the
                    local (deleted) object won't be rejected.
     */
    /* create sorted array of XIDelCmd-items, and unify it */
    /* in case of pruning set to OPT_OFF, this sorting/unifying
       step is done lateron. */
    arrayXIDelCmd = SortedArrayXIDelCmd(context, sort_XIDelCmd);
    if (arrayXIDelCmd==NULL && ctx.nXIDelCmd>0)
    {
      Dune::dwarn << "out of memory in DDD_XferEnd(), giving up.\n";
      ret_code = DDD_RET_ERROR_NOMEM;
      LC_Abort(context, EXCEPTION_LOWCOMM_USER);
      goto exit;
    }
    remXIDelCmd   = UnifyXIDelCmd(context, arrayXIDelCmd, unify_XIDelCmd);
    obsolete += (ctx.nXIDelCmd-remXIDelCmd);

    /* do communication and actual pruning */
    prunedXIDelCmd = PruneXIDelCmd(context, arrayXIDelCmd, remXIDelCmd, arrayXICopyObj);
    obsolete += prunedXIDelCmd;
    remXIDelCmd -= prunedXIDelCmd;

    DelCmds_were_pruned = true;
  }
  else
  {
    DelCmds_were_pruned = false;
  }

  /*
     if (nXIDelCmd>0||remXIDelCmd>0||prunedXIDelCmd>0)
     printf("%4d: XIDelCmd. all=%d rem=%d pruned=%d\n", me,
     nXIDelCmd, remXIDelCmd, prunedXIDelCmd);
   */


  /*
          COMMUNICATION PHASE 1
   */
  STAT_RESET;
  /* send Cpl-info about new objects to owners of other local copies */
  arrayNewOwners = CplClosureEstimate(context, arrayXICopyObj, &nNewOwners);
  if (nNewOwners>0 && arrayNewOwners==NULL)
  {
    Dune::dwarn << "out of memory in DDD_XferEnd(), giving up.\n";
    ret_code = DDD_RET_ERROR_NOMEM;
    LC_Abort(context, EXCEPTION_LOWCOMM_USER);
    goto exit;
  }

  /* create sorted array of XINewCpl- and XIOldCpl-items.
     TODO. if efficiency is a problem here, use b-tree or similar
           data structure to improve performance. */
  arrayXINewCpl = SortedArrayXINewCpl(context, sort_XINewCpl);
  if (arrayXINewCpl==NULL && ctx.nXINewCpl>0)
  {
    Dune::dwarn << "out of memory in DDD_XferEnd(), giving up.\n";
    ret_code = DDD_RET_ERROR_NOMEM;
    LC_Abort(context, EXCEPTION_LOWCOMM_USER);
    goto exit;
  }

  arrayXIOldCpl = SortedArrayXIOldCpl(context, sort_XIOldCpl);
  if (arrayXIOldCpl==NULL && ctx.nXIOldCpl>0)
  {
    Dune::dwarn << "out of memory in DDD_XferEnd(), giving up.\n";
    ret_code = DDD_RET_ERROR_NOMEM;
    LC_Abort(context, EXCEPTION_LOWCOMM_USER);
    goto exit;
  }


  /* prepare msgs for objects and XINewCpl-items */
  PrepareObjMsgs(context,
                 arrayXICopyObj,
                 arrayXINewCpl, ctx.nXINewCpl,
                 arrayXIOldCpl, ctx.nXIOldCpl,
                 &sendMsgs, &sendMem);


  /*
     DisplayMemResources(context);
   */

  /* init communication topology */
  nRecvMsgs = LC_Connect(context, ctx.objmsg_t);
  STAT_TIMER(T_XFER_PREP_MSGS);
  if (nRecvMsgs<0)
  {
    /* some processor raised an exception */
    if (nRecvMsgs==EXCEPTION_LOWCOMM_CONNECT)
    {
      /* the dangerous exception: it occurred only locally,
         the other procs doesn't know about it */
      Dune::dwarn << "local exception during LC_Connect() in DDD_XferEnd(), giving up.\n";

      /* in this state the local processor hasn't initiated any send
         or receive calls. however, there may be (and almost ever:
         there will be!) other processors which already initiated their
         receive calls. This is a tragic situation without a possibility
         to escape.
       */
      HARD_EXIT;
    }
    else
    {
      /* all other exceptions are known globally, shutdown safely */
      Dune::dwarn << "error during LC_Connect() in DDD_XferEnd(), giving up.\n";
      ret_code = DDD_RET_ERROR_UNKNOWN;
      goto exit;
    }
  }

  /*** all exceptional errors which occur from here down to the
       point of no return (some lines below) could be cleaned up
       locally, but the communication situation cannot be cleaned
       up with the current functionality of PPIF (i.e., discarding
       of pending communication calls). therefore, the local processor
       will be able to shutdown safely, but other processors might hang. ***/

  STAT_RESET;
  /* build obj msgs on sender side and start send */
  if (! IS_OK(XferPackMsgs(context, sendMsgs)))
  {
    Dune::dwarn << "error during message packing in DDD_XferEnd(), giving up.\n";
    LC_Cleanup(context);
    ret_code = DDD_RET_ERROR_UNKNOWN;
    goto exit;
  }
  STAT_TIMER(T_XFER_PACK_SEND);

  /*
          now messages are in the net, use spare time
   */

  /* create sorted array of XISetPrio-items, and unify it */
  STAT_RESET;
  arrayXISetPrio = XISetPrioSet_GetArray(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio));
  obsolete += XISetPrioSet_GetNDiscarded(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio));


  if (!DelCmds_were_pruned)
  {
    /* create sorted array of XIDelCmd-items, and unify it */
    arrayXIDelCmd = SortedArrayXIDelCmd(context, sort_XIDelCmd);
    if (arrayXIDelCmd==NULL && ctx.nXIDelCmd>0)
    {
      Dune::dwarn << "out of memory in DDD_XferEnd(), giving up.\n";
      LC_Cleanup(context);
      ret_code = DDD_RET_ERROR_NOMEM;
      goto exit;
    }
    remXIDelCmd   = UnifyXIDelCmd(context, arrayXIDelCmd, unify_XIDelCmd);
    obsolete += (ctx.nXIDelCmd-remXIDelCmd);
  }


  /*** this is the point of no return. the next function manipulates
       the data structure irreversibly.                              ***/

  /* execute local commands */
  /* NOTE: messages have been build before in order to allow
           deletion of objects. */
  ExecLocalXIDelCmd(context, arrayXIDelCmd,  remXIDelCmd);

  /* now all XIDelObj-items have been created. these come from:
          1. application->DDD_XferDeleteObj->XIDelCmd->HdrDestructor->
             ->XferRegisterDelete
          2. HANDLER_DELETE->HdrDestructor (for dependent object)->
             ->XferRegisterDelete
   */

  /* create sorted array of XIDelObj-items */
  arrayXIDelObj = SortedArrayXIDelObj(context, sort_XIDelObj);

  ExecLocalXISetPrio(context, arrayXISetPrio,
                     arrayXIDelObj,  ctx.nXIDelObj,
                     arrayNewOwners, nNewOwners);
  ExecLocalXIDelObj(context,
                    arrayXIDelObj,  ctx.nXIDelObj,
                    arrayNewOwners, nNewOwners);


  if (obsolete>0)
  {
    if (DDD_GetOption(context, OPT_INFO_XFER) & XFER_SHOW_OBSOLETE)
    {
      int all = ctx.nXIDelObj+
                XISetPrioSet_GetNItems(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio))+
                XICopyObjSet_GetNItems(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj));

      using std::setw;
      Dune::dwarn
        << "DDD MESG [" << setw(3) << me << "]: " << setw(4) << obsolete
        << " from " << setw(4) << all << " xfer-cmds obsolete.\n";
    }
  }
  STAT_TIMER(T_XFER_WHILE_COMM);

  /*
          nothing more to do until incoming messages arrive
   */

  /* display information about send-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_XFER) & XFER_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD XFER_SHOW_MSGSALL: ObjMsg.Send\n";
    LC_PrintSendMsgs(context);
  }


  /* wait for communication-completion (send AND receive) */
  STAT_RESET;
  recvMsgs = LC_Communicate(context);
  STAT_TIMER(T_XFER_WAIT_RECV);


  /* display information about message buffer sizes */
  if (DDD_GetOption(context, OPT_INFO_XFER) & XFER_SHOW_MEMUSAGE)
  {
    int k;

    /* sum up sizes of receive mesg buffers */
    for(k=0; k<nRecvMsgs; k++)
    {
      recvMem += LC_GetBufferSize(recvMsgs[k]);
    }

    using std::setw;
    Dune::dwarn
      << "DDD MESG [" << setw(3) << me << "]: SHOW_MEM msgs "
      << " send=" << setw(10) << sendMem
      << " recv=" << setw(10) << recvMem
      << " all=" << setw(10) << (sendMem+recvMem) << "\n";
  }

  /* display information about recv-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_XFER) & XFER_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD XFER_SHOW_MSGSALL: ObjMsg.Recv\n";
    LC_PrintRecvMsgs(context);
  }


  /* get sorted list of local objects with couplings */
  localCplObjs = LocalCoupledObjectsList(context);


  /* unpack messages */
  STAT_RESET;
  XferUnpack(context, recvMsgs, nRecvMsgs,
             localCplObjs.data(), nCpls,
             arrayXISetPrio,
             arrayXIDelObj, ctx.nXIDelObj,
             arrayXICopyObj,
             arrayNewOwners, nNewOwners);
  LC_Cleanup(context);
  STAT_TIMER(T_XFER_UNPACK);

  /* recreate sorted list of local coupled objects,
     old list might be corrupt due to creation of new objects */
  STAT_RESET;
  localCplObjs = LocalCoupledObjectsList(context);


  /* create sorted array of XIDelCpl-, XIModCpl- and XIAddCpl-items.
     TODO. if efficiency is a problem here, use b-tree or similar
           data structure to improve performance. */
  arrayXIDelCpl = SortedArrayXIDelCpl(context, sort_XIDelCpl);
  arrayXIModCpl = SortedArrayXIModCpl(context, sort_XIModCpl);
  arrayXIAddCpl = SortedArrayXIAddCpl(context, sort_XIAddCpl);


  /* some XIDelCpls have been invalidated by UpdateCoupling(),
     decrease list size to avoid sending them */
  remXIDelCpl = ctx.nXIDelCpl;
  while (remXIDelCpl>0 && arrayXIDelCpl[remXIDelCpl-1]->to == procs)
    remXIDelCpl--;

  remXIModCpl   = UnifyXIModCpl(context, arrayXIModCpl, unify_XIModCpl);
  STAT_TIMER(T_XFER_PREP_CPL);

  /*
     printf("%4d: %d XIDelCpls obsolete\n", me, nXIDelCpl-remXIDelCpl);
   */


  /*
          COMMUNICATION PHASE 2
   */

  STAT_RESET;
  CommunicateCplMsgs(context,
                     arrayXIDelCpl, remXIDelCpl,
                     arrayXIModCpl, remXIModCpl,
                     arrayXIAddCpl, ctx.nXIAddCpl,
                     localCplObjs.data(), nCpls);
  STAT_TIMER(T_XFER_CPLMSG);


  /*
          CLEAN-UP PHASE 2
   */
exit:

  /* free temporary storage */
  XICopyObjSet_Reset(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj));

  if (arrayNewOwners!=NULL) OO_Free (arrayNewOwners /*,0*/);
  FreeAllXIAddData(context);

  XISetPrioSet_Reset(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio));

  if (arrayXIDelCmd!=NULL) OO_Free (arrayXIDelCmd /*,0*/);
  FreeAllXIDelCmd(context);

  if (arrayXIDelObj!=NULL) OO_Free (arrayXIDelObj /*,0*/);
  FreeAllXIDelObj(context);

  if (arrayXINewCpl!=NULL) OO_Free (arrayXINewCpl /*,0*/);
  FreeAllXINewCpl(context);

  if (arrayXIOldCpl!=NULL) OO_Free (arrayXIOldCpl /*,0*/);
  FreeAllXIOldCpl(context);

  if (arrayXIDelCpl!=NULL) OO_Free (arrayXIDelCpl /*,0*/);
  FreeAllXIDelCpl(context);

  if (arrayXIModCpl!=NULL) OO_Free (arrayXIModCpl /*,0*/);
  FreeAllXIModCpl(context);

  if (arrayXIAddCpl!=NULL) OO_Free (arrayXIAddCpl /*,0*/);
  FreeAllXIAddCpl(context);

  for(; sendMsgs!=NULL; sendMsgs=sm)
  {
    sm = sendMsgs->next;
    OO_Free (sendMsgs /*,0*/);
  }

#       if DebugXfer<=4
  Dune::dverb << "XferEnd, before IFAllFromScratch().\n";
#       endif

  if (ret_code==DDD_RET_OK)
  {
    /* re-create all interfaces and step XMODE */
    STAT_RESET;
    IFAllFromScratch(context);
    STAT_TIMER(T_XFER_BUILD_IF);
  }

  XferStepMode(context, DDD::Xfer::XferMode::XMODE_BUSY);
  return(ret_code);
}




/* ablage fuer debug-ausgabe

 #	if DebugXfer<=4
                sprintf(cBuffer,"%4d: XferEnd, after XferDeleteObjects(): "
                        "send=%d recv=%d\n", me, nSendMsgs, nRecvMsgs);
                DDD_PrintDebug(cBuffer);
 #	endif
 */



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferPrioChange                                            */
/*                                                                          */
/****************************************************************************/

/**
        Consistent change of a local object's priority during DDD Transfer.
        Local objects which are part of a distributed object must notify
        other copies about local priority changes. This is accomplished
        by issuing \funk{XferPrioChange}-commands during the transfer phase;
        DDD will send appropriate messages to the owner processors of
        the other copies.

        This function is regarded as a {\bf Transfer}-operation due
        to its influence on DDD management information on neighbouring
        processors. Therefore the function has to be issued between
        a starting \funk{XferBegin} and a final \funk{XferEnd} call.

   @param hdr  DDD local object whose priority should be changed.
   @param prio new priority of that local object.
 */

void DDD_XferPrioChange (DDD::DDDContext& context, DDD_HDR hdr, DDD_PRIO prio)
{
  auto& ctx = context.xferContext();

  XISetPrio *xi = XISetPrioSet_NewItem(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio));
  xi->hdr  = hdr;
  xi->gid  = OBJ_GID(hdr);
  xi->prio = prio;

  if (! XISetPrioSet_ItemOK(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio)))
    return;

#       if DebugXfer<=2
  Dune::dvverb << "DDD_XferPrioChange " << OBJ_GID(hdr) << ", prio=" << prio << "\n";
#       endif
}



static void XferInitCopyInfo (DDD::DDDContext& context,
                              DDD_HDR hdr,
                              TYPE_DESC *desc,
                              size_t size,
                              DDD_PROC dest,
                              DDD_PRIO prio)
{
  auto& ctx = context.xferContext();

  if (!ddd_XferActive(context))
    DUNE_THROW(Dune::Exception, "Missing DDD_XferBegin()");

  if (dest >= context.procs())
    DUNE_THROW(Dune::Exception,
               "cannot transfer " << OBJ_GID(hdr) << " to processor " << dest
               << " (procs=" << context.procs() << ")");

  if (prio>=MAX_PRIO)
    DUNE_THROW(Dune::Exception,
               "priority must be less than " << MAX_PRIO
               << " (prio=" << prio << ")");

  if (dest==context.me())
  {
    /* XFER-C4: XferCopyObj degrades to SetPrio command */
    XISetPrio *xi = XISetPrioSet_NewItem(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio));
    xi->hdr  = hdr;
    xi->gid  = OBJ_GID(hdr);
    xi->prio = prio;

    if (! XISetPrioSet_ItemOK(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio)))
    {
      /* item has been inserted already, don't store it twice. */
      /* even don't call XFERCOPY-handler, this is a real API change! */

      /* xi->prio will be set to PRIO_INVALID if the priority of
         the previously existing XICopyObj-item wins the PriorityMerge
         in the corresponding Compare function (see supp.c). Then,
         we in fact won't need calling the XFERCOPY-handler here,
         because it doesn't give new information. If xi->prio is
         not PRIO_INVALID, then the XICopyObj-item xi wins the merge
         and the XFERCOPY-handler has to be called a second time, now
         with a higher priority. */
      if (xi->prio==PRIO_INVALID)
        return;
    }


    /* although XferCopyObj degrades to SetPrio, call XFERCOPY-handler! */

    /* reset for eventual AddData-calls during handler execution */
    ctx.theXIAddData = nullptr;

    /* call application handler for xfer of dependent objects */
    if (desc->handlerXFERCOPY)
    {
      DDD_OBJ obj = HDR2OBJ(hdr,desc);

      desc->handlerXFERCOPY(context, obj, dest, prio);
    }

    /* theXIAddData might be changed during handler execution */
    ctx.theXIAddData = nullptr;
  }
  else
  {
    /* this is a real transfer to remote proc */
    XICopyObj  *xi = XICopyObjSet_NewItem(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj));
    xi->hdr  = hdr;
    xi->gid  = OBJ_GID(hdr);
    xi->dest = dest;
    xi->prio = prio;

    if (! XICopyObjSet_ItemOK(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj)))
    {
      /* item has been inserted already, don't store it twice. */
      /* even don't call XFERCOPY-handler, this is a real API change! */

      /* xi->prio will be set to PRIO_INVALID if the priority of
         the previously existing XICopyObj-item wins the PriorityMerge
         in the corresponding Compare function (see supp.c). Then,
         we in fact won't need calling the XFERCOPY-handler here,
         because it doesn't give new information. If xi->prio is
         not PRIO_INVALID, then the XICopyObj-item xi wins the merge
         and the XFERCOPY-handler has to be called a second time, now
         with a higher priority. */
      if (xi->prio==PRIO_INVALID)
        return;
    }

    xi->size = size;
    xi->add    = NULL;
    xi->addLen = 0;

    /* set XferAddInfo for evtl AddData-calls during handler execution */
    ctx.theXIAddData = xi;

    /* call application handler for xfer of dependent objects */
    if (desc->handlerXFERCOPY)
    {
      DDD_OBJ obj = HDR2OBJ(hdr,desc);

      desc->handlerXFERCOPY(context, obj, dest, prio);
    }

    /* theXIAddData might be changed during handler execution */
    ctx.theXIAddData = xi;
  }
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferCopyObj                                               */
/*                                                                          */
/****************************************************************************/

/**
        Transfer-command for copying a local DDD object to another processor.
        After an initial call to \funk{XferBegin}, this function
        creates a copy of one local DDD object on another processor with a certain
        priority. The necessary actions (packing/unpacking of object data, message
        transfer) are executed via the final call to \funk{XferEnd}; therefore
        a whole set of {\bf Transfer}-operations is accumulated.

        Caution: As the original object data is not copied throughout this
        call due to efficiency reasons (transferring a large number of objects
        would result in a huge amount of memory copy operations),
        the object may not be changed or deleted until the actual transfer
        has happened. Otherwise the changes will be sent, too.

        Two different mechanisms allow the transfer of data depending on the
        stated object:

        \begin{itemize}
        \item Right after the function \funk{XferCopyObj} has been called,
        the optional handler #HANDLER_XFERCOPY# is executed by DDD.
        Basically this mechanism allows the transfer of dependent DDD objects
        (or: hierarchies of objects), although arbitrary actions may occur
        inside the handler.
        No relationship between the {\em primary} object and the additional
        objects can be expressed, as many different destination processors
        might be involved.
        %
        \item Via an arbitrary number of additional calls to \funk{XferAddData}
        arrays of {\em data objects} (i.e.~without the usual DDD object header)
        may be sent.
        Using this mechanism the data objects are strongly linked to the
        {\em primary} object; all data objects are transferred to the same
        destination processor.
        \end{itemize}

        After the object copy has been established on the destination processor,
        first the optional handler #HANDLER_LDATACONSTRUCTOR# is called to
        update the LDATA parts of that object (e.g.~locally linked lists).
        Afterwards the optional handler #HANDLER_OBJMKCONS# is called
        to establish consistency for that object (e.g.~backward references on
        the new object copy). For handlers related to function
        \funk{XferAddData} refer to its description.

   @param hdr   DDD local object which has to be copied.
   @param proc  destination processor which will receive the object copy.
   @param prio  DDD priority of new object copy.
 */

void DDD_XferCopyObj (DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc, DDD_PRIO prio)
{
  TYPE_DESC *desc =  &context.typeDefs()[OBJ_TYPE(hdr)];

#       if DebugXfer<=2
  Dune::dvverb << "DDD_XferCopyObj " << OBJ_GID(hdr)
               << ", proc=" << proc << " prio=" << prio << "\n";
#       endif

XferInitCopyInfo(context, hdr, desc, desc->size, proc, prio);
}





/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferCopyObjX                                              */
/*                                                                          */
/****************************************************************************/

/**
        Transfer-command for objects of varying sizes.
        This function is an extension of \funk{XferCopyObj}.
        For objects with same DDD type but with variable size in memory,
        one can give the real size as forth parameter to \funk{XferCopyObjX}.
        The DDD Transfer module will use that size value instead of using
        the size computed in \funk{TypeDefine} after the definition of
        the object's DDD type.

   @param hdr   DDD local object which has to be copied.
   @param proc  destination processor which will receive the object copy.
   @param prio  DDD priority of new object copy.
   @param size  real size of local object.
 */
void DDD_XferCopyObjX (DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc, DDD_PRIO prio, size_t size)
{
  TYPE_DESC *desc =  &context.typeDefs()[OBJ_TYPE(hdr)];

#       if DebugXfer<=2
  Dune::dvverb
    << "DDD_XferCopyObjX " << OBJ_GID(hdr) << ", proc=" << proc
    << " prio=" << prio << " size=" << size << "\n";
#       endif

  if ((desc->size!=size) && (DDD_GetOption(context, OPT_WARNING_VARSIZE_OBJ)==OPT_ON))
    Dune::dwarn << "object size differs from declared size in DDD_XferCopyObjX\n";

  if ((desc->size>size) && (DDD_GetOption(context, OPT_WARNING_SMALLSIZE)==OPT_ON))
    Dune::dwarn << "object size smaller than declared size in DDD_XferCopyObjX\n";

  XferInitCopyInfo(context, hdr, desc, size, proc, prio);
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferAddData                                               */
/*                                                                          */
/****************************************************************************/

/**
        Transfer array of additional data objects with a DDD local object.
        This function transfers an array of additional data objects
        corresponding to one DDD object to the same destination processor.
        Therefore the latest call to \funk{XferCopyObj} defines the
        {\em primary} object and the destination. An arbitrary number of
        \funk{XferAddData}-calls may be issued to transfer various
        data object arrays at once. This serves as a mechanism to send
        object references which cannot be handled by the native DDD
        pointer conversion technique.

        Just before the actual {\bf Transfer}-operation is executed, the (optional)
        handler #HANDLER_XFERGATHER# will be called to fill the data
        object array into some reserved storage space.
        After the {\bf Xfer}-operation the handler #HANDLER_XFERSCATTER# is
        called on the receiving processor to rebuild the data objects.

        As the data objects had to be registered and equipped with a
        usual {\em type\_id} (as with the DDD objects), the
        standard DDD pointer conversion will also take place for the data objects.

   @param cnt  number of data objects in array (i.e.~array length).
   @param typ  DDD type of data objects. All data objects inside one array
        should have the same object type. This object type is defined by
        registering the object structure via \funk{TypeDefine} as usual,
        but without including the DDD object header.
 */

void DDD_XferAddData (DDD::DDDContext& context, int cnt, DDD_TYPE typ)
{
  auto& ctx = context.xferContext();
  XFERADDDATA *xa;

#       if DebugXfer<=2
  Dune::dvverb << "DDD_XferAddData cnt=" << cnt << " typ=" << typ << "\n";
#       endif

  if (not ctx.theXIAddData)
    return;

  xa = NewXIAddData(context);
  if (xa==NULL)
    throw std::bad_alloc();

  xa->addCnt = cnt;
  xa->addTyp = typ;
  xa->sizes  = NULL;

  if (typ<DDD_USER_DATA || typ>DDD_USER_DATA_MAX)
  {
    /* normal dependent object */
    const TYPE_DESC& descDepTyp = context.typeDefs()[typ];

    xa->addLen       = CEIL(descDepTyp.size)   * cnt;
    xa->addNPointers = (descDepTyp.nPointers) * cnt;
  }
  else
  {
    /* stream of bytes, since V1.2 */
    /* many streams, since V1.7.8 */
    xa->addLen       = CEIL(cnt);
    xa->addNPointers = 0;
  }

  ctx.theXIAddData->addLen += xa->addLen;
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferAddDataX                                              */
/*                                                                          */
/****************************************************************************/

/**
        Transfer array of additional, variable-sized data objects.
        \todo{not documented yet.}
 */

void DDD_XferAddDataX (DDD::DDDContext& context, int cnt, DDD_TYPE typ, size_t *sizes)
{
  auto& ctx = context.xferContext();
  XFERADDDATA *xa;
  int i;

#       if DebugXfer<=2
  Dune::dvverb << "DDD_XferAddData cnt=" << cnt << " typ=" << typ << "\n";
#       endif

  if (not ctx.theXIAddData)
    return;

  xa = NewXIAddData(context);
  if (xa==NULL)
    HARD_EXIT;

  xa->addCnt = cnt;
  xa->addTyp = typ;

  if (typ<DDD_USER_DATA || typ>DDD_USER_DATA_MAX)
  {
    /* copy sizes array */
    xa->sizes = AddDataAllocSizes(context, cnt);
    memcpy(xa->sizes, sizes, sizeof(int)*cnt);

    /* normal dependent object */
    const TYPE_DESC& descDepTyp = context.typeDefs()[typ];

    xa->addLen = 0;
    for (i=0; i<cnt; i++)
    {
      xa->addLen += CEIL(sizes[i]);
    }
    xa->addNPointers = descDepTyp.nPointers * cnt;
  }
  else
  {
    /* stream of bytes, since V1.2 */
    /* many streams, since V1.7.8 */
    xa->addLen       = CEIL(cnt);
    xa->addNPointers = 0;
  }

  ctx.theXIAddData->addLen += xa->addLen;
}


/**
        Tell application if additional data will be sent.
        If the application issues a \funk{XferCopyObj} command
        with the local processor as the destination processor,
        no object copy will be created. Therefore, also additional
        data objects will not be sent and the corresponding
   #XFER_GATHER#/#XFER_SCATTER# handlers will not be called.
        This function returns a boolean value indicating whether
        the last \funk{XferCopyObj}-command will need the specification
        of additional data objects.

        The application program may use this function in order to
        avoid unnecessary work, \eg, for counting the number of
        additional data objects.

   @return #true# if additional data objects will be gathered, sent
                and scattered; #false# otherwise.
 */
bool DDD_XferWithAddData(const DDD::DDDContext& context)
{
  /* if theXIAddData==NULL, the XferAddData-functions will
     do nothing -> the Gather/Scatter-handlers will not be
     called. */
  return context.xferContext().theXIAddData != nullptr;
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferDeleteObj                                             */
/*                                                                          */
/****************************************************************************/

/**
        Transfer-command for deleting a local DDD object.
        This function is regarded as a {\bf Transfer}-operation due
        to its influence on DDD management information on neighbouring
        processors. Therefore the function has to be issued between
        a starting \funk{XferBegin} and a final \funk{XferEnd} call.

        During the actual {\bf Transfer}-operation all data corresponding
        to that object and the object memory itself will be deleted.

   @param hdr   DDD local object which has to be deleted.
 */

void DDD_XferDeleteObj (DDD::DDDContext& context, DDD_HDR hdr)
{
  TYPE_DESC *desc =  &context.typeDefs()[OBJ_TYPE(hdr)];
  XIDelCmd  *dc = NewXIDelCmd(context);

  if (dc==NULL)
    HARD_EXIT;

  dc->hdr = hdr;

#       if DebugXfer<=2
  Dune::dvverb << "DDD_XferDeleteObj " << OBJ_GID(hdr) << "\n";
#       endif


  /* call application handler for deletion of dependent objects */
  if (desc->handlerXFERDELETE!=NULL)
  {
    desc->handlerXFERDELETE(context, HDR2OBJ(hdr,desc));
  }
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferBegin                                                 */
/*                                                                          */
/****************************************************************************/

/**
        Starts transfer phase.
        A call to this function establishes a global transfer operation.
        It should be issued on all processors. After this call an arbitrary
        series of {\bf Xfer}-commands may be issued. The global transfer operation
        is carried out via a \funk{XferEnd} call on each processor.
 */

void DDD_XferBegin(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();
  ctx.theXIAddData = nullptr;


  /* step mode and check whether call to XferBegin is valid */
  if (!XferStepMode(context, DDD::Xfer::XferMode::XMODE_IDLE))
    DUNE_THROW(Dune::Exception, "DDD_XferBegin() aborted");
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferIsPrunedDelete                                        */
/*                                                                          */
/****************************************************************************/

/**
    Returns information about pruned \funk{XferDeleteObj} command.
    If a \funk{XferDeleteObj} command has been pruned (i.e., option
   #OPT_XFER_PRUNE_DELETE# is set to #OPT_ON# and another processor issued
    a \funk{XferCopyObj}, command which sends an object copy to the
    local processor), then this function will return #XFER_PRUNED_TRUE#,
    otherwise it returns #XFER_PRUNED_FALSE#. If an error condition
    occurs (e.g., when it is called at the wrong time), the function
    returns #XFER_PRUNED_ERROR#.

   @param hdr   DDD local object which has to be deleted.
   @return  one of #XFER_PRUNED_xxx#
 */


int DDD_XferIsPrunedDelete(const DDD::DDDContext& context, DDD_HDR hdr)
{
  if (XferMode(context) != DDD::Xfer::XferMode::XMODE_BUSY)
  {
    return(XFER_PRUNED_ERROR);
  }

  if (OBJ_PRUNED(hdr))
    return(XFER_PRUNED_TRUE);

  return(XFER_PRUNED_FALSE);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_XferObjIsResent                                           */
/*                                                                          */
/****************************************************************************/

#ifdef SUPPORT_RESENT_FLAG

/**
    Returns if an object will receive an additional copy.
    If another processor issued a \funk{XferCopyObj} in order
    to send a further copy of the given local object to the local
    processor, this function will return #XFER_RESENT_TRUE#.
    Otherwise this function will return #XFER_RESENT_FALSE#.
    If an error condition occurs (e.g., when it is called at the
    wrong time), the function returns #XFER_RESENT_ERROR#.

    This function will only work with option #OPT_XFER_PRUNE_DELETE#
    set to #OPT_ON#. Otherwise, there will be no kind of communication
    to supply this information.

   @param hdr   DDD local object which has to be deleted.
   @return  one of #XFER_RESENT_xxx#
 */

int DDD_XferObjIsResent(const DDD::DDDContext& context, DDD_HDR hdr)
{
  if (XferMode(context) != DDD::Xfer::XferMode::XMODE_BUSY)
  {
    return(XFER_RESENT_ERROR);
  }

  if (DDD_GetOption(context, OPT_XFER_PRUNE_DELETE)==OPT_OFF)
  {
    return(XFER_RESENT_ERROR);
  }

  if (OBJ_RESENT(hdr))
    return(XFER_RESENT_TRUE);

  return(XFER_RESENT_FALSE);
}

#endif /* SUPPORT_RESENT_FLAG */




END_UGDIM_NAMESPACE
