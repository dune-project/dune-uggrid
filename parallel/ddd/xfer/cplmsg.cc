// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      cplmsg.c                                                      */
/*                                                                          */
/* Purpose:   ddd object transfer:                                          */
/*               last messages in order to create coupling consistency.     */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   960712 kb  created                                            */
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

#include <forward_list>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <parallel/ddd/dddi.h>
#include "xfer.h"


USING_UG_NAMESPACE
using namespace PPIF;

  START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

/* CPLMSG: complete description of message on sender side */

struct CPLMSG
{
  DDD_PROC proc;

  XIDelCpl  **xferDelCpl = nullptr;
  int nDelCpl = 0;

  XIModCpl  **xferModCpl = nullptr;
  int nModCpl = 0;

  XIAddCpl  **xferAddCpl = nullptr;
  int nAddCpl = 0;


  /* lowcomm message handle */
  LC_MSGHANDLE msg_h;

  CPLMSG(DDD_PROC dest)
    : proc(dest)
    {}
};

using CplmsgList = std::forward_list<CPLMSG>;

/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


void CplMsgInit(DDD::DDDContext& context)
{
  auto& ctx = context.cplmsgContext();

  ctx.cplmsg_t = LC_NewMsgType(context, "CplMsg");
  ctx.delcpl_id = LC_NewMsgTable("DelCpl", ctx.cplmsg_t, sizeof(TEDelCpl));
  ctx.modcpl_id = LC_NewMsgTable("ModCpl", ctx.cplmsg_t, sizeof(TEModCpl));
  ctx.addcpl_id = LC_NewMsgTable("AddCpl", ctx.cplmsg_t, sizeof(TEAddCpl));
}


void CplMsgExit(DDD::DDDContext&)
{}


/****************************************************************************/

static
std::pair<int, CplmsgList>
PrepareCplMsgs (
  DDD::DDDContext& context,
  XIDelCpl **itemsDC, int nDC,
  XIModCpl **itemsMC, int nMC,
  XIAddCpl **itemsAC, int nAC)
{
  auto& ctx = context.cplmsgContext();
  const auto procs = context.procs();

#       if DebugCplMsg<=3
  Dune::dverb << "PrepareCplMsgs, nXIDelCpl=" << nDC << " nXIModCpl=" << nMC
              << " nXIAddCpl=" << nAC << "\n";
#       endif

  CplmsgList msgs;
  int nMsgs = 0;

  auto msgForProc = [&](DDD_PROC dest) -> CPLMSG& {
    if (msgs.empty() or msgs.front().proc != dest) {
      msgs.emplace_front(dest);
      ++nMsgs;
    }
    return msgs.front();
  };

  /*
          run through all tables simultaneously,
          each time a new proc-nr is encountered in one of these
          tables, create a new CPLMSG item.

          (the lists have been sorted according to proc-nr previously.)
   */

  int iDC=0, iMC=0, iAC=0;
  while (iDC<nDC || iMC<nMC || iAC<nAC)
  {
    DDD_PROC pDC = (iDC<nDC) ? itemsDC[iDC]->to   : procs;
    DDD_PROC pMC = (iMC<nMC) ? itemsMC[iMC]->to   : procs;
    DDD_PROC pAC = (iAC<nAC) ? itemsAC[iAC]->to   : procs;

    /* check DelCpl-items */
    if (pDC<=pMC && pDC<=pAC && pDC<procs)
    {
      int i;

      CPLMSG& xm = msgForProc(pDC);
      xm.xferDelCpl = itemsDC+iDC;
      for(i=iDC; i<nDC && itemsDC[i]->to==pDC; i++)
        ;

      xm.nDelCpl = i-iDC;
      iDC = i;
    }

    /* check ModCpl-items */
    if (pMC<=pDC && pMC<=pAC && pMC<procs)
    {
      int i;

      CPLMSG& xm = msgForProc(pMC);
      xm.xferModCpl = itemsMC+iMC;
      for(i=iMC; i<nMC && itemsMC[i]->to==pMC; i++)
        ;

      xm.nModCpl = i-iMC;
      iMC = i;
    }

    /* check AddCpl-items */
    if (pAC<=pDC && pAC<=pMC && pAC<procs)
    {
      int i;

      CPLMSG& xm = msgForProc(pAC);
      xm.xferAddCpl = itemsAC+iAC;
      for(i=iAC; i<nAC && itemsAC[i]->to==pAC; i++)
        ;

      xm.nAddCpl = i-iAC;
      iAC = i;
    }

    if (pDC==procs) iDC = nDC;
    if (pMC==procs) iMC = nMC;
    if (pAC==procs) iAC = nAC;
  }


  /* initiate send messages */
  for(auto& xm : msgs)
  {
    /* create new send message */
    xm.msg_h = LC_NewSendMsg(context, ctx.cplmsg_t, xm.proc);

    /* init tables inside message */
    LC_SetTableSize(xm.msg_h, ctx.delcpl_id, xm.nDelCpl);
    LC_SetTableSize(xm.msg_h, ctx.modcpl_id, xm.nModCpl);
    LC_SetTableSize(xm.msg_h, ctx.addcpl_id, xm.nAddCpl);

    /* prepare message for sending away */
    LC_MsgPrepareSend(context, xm.msg_h);
  }

  return {nMsgs, std::move(msgs)};
}


static void CplMsgSend(DDD::DDDContext& context, const CplmsgList& msgs)
{
  auto& ctx = context.cplmsgContext();

  for(const auto& msg : msgs)
  {
    TEDelCpl *arrayDC = (TEDelCpl *)LC_GetPtr(msg.msg_h, ctx.delcpl_id);
    TEModCpl *arrayMC = (TEModCpl *)LC_GetPtr(msg.msg_h, ctx.modcpl_id);
    TEAddCpl *arrayAC = (TEAddCpl *)LC_GetPtr(msg.msg_h, ctx.addcpl_id);

    /* copy data into message */
    for(int i=0; i < msg.nDelCpl; i++)
    {
      arrayDC[i] = msg.xferDelCpl[i]->te;
    }
    for(int i=0; i < msg.nModCpl; i++)
    {
      arrayMC[i] = msg.xferModCpl[i]->te;
    }
    for(int i=0; i < msg.nAddCpl; i++)
    {
      arrayAC[i] = msg.xferAddCpl[i]->te;
    }

    /* schedule message for send */
    LC_MsgSend(context, msg.msg_h);
  }
}



/****************************************************************************/


static void CplMsgUnpackSingle (DDD::DDDContext& context, LC_MSGHANDLE xm,
                                DDD_HDR *localCplObjs, int nLCO)
{
  auto& ctx = context.cplmsgContext();

  TEDelCpl  *theDelCpl;
  TEModCpl  *theModCpl;
  TEAddCpl  *theAddCpl;
  int i, j, nDelCpl, nModCpl, nAddCpl;
  DDD_PROC proc = LC_MsgGetProc(xm);

  /* get number and address of del-items */
  nDelCpl = (int) LC_GetTableLen(xm, ctx.delcpl_id);
  nModCpl = (int) LC_GetTableLen(xm, ctx.modcpl_id);
  nAddCpl = (int) LC_GetTableLen(xm, ctx.addcpl_id);
  theDelCpl = (TEDelCpl *) LC_GetPtr(xm, ctx.delcpl_id);
  theModCpl = (TEModCpl *) LC_GetPtr(xm, ctx.modcpl_id);
  theAddCpl = (TEAddCpl *) LC_GetPtr(xm, ctx.addcpl_id);


  /* modify couplings according to mod-list */
  for(i=0, j=0; i<nModCpl; i++)
  {
    while ((j<nLCO) && (OBJ_GID(localCplObjs[j]) < theModCpl[i].gid))
      j++;

    if ((j<nLCO) && (OBJ_GID(localCplObjs[j])==theModCpl[i].gid))
    {
      ModCoupling(context, localCplObjs[j], proc, theModCpl[i].prio);
    }
  }


  /* delete couplings according to del-list */
  for(i=0, j=0; i<nDelCpl; i++)
  {
    while ((j<nLCO) && (OBJ_GID(localCplObjs[j]) < theDelCpl[i].gid))
      j++;

    if ((j<nLCO) && (OBJ_GID(localCplObjs[j])==theDelCpl[i].gid))
    {
      DelCoupling(context, localCplObjs[j], proc);
    }
  }


  /* add couplings according to add-list */
  for(i=0, j=0; i<nAddCpl; i++)
  {
    while ((j<nLCO) && (OBJ_GID(localCplObjs[j]) < theAddCpl[i].gid))
      j++;

    if ((j<nLCO) && (OBJ_GID(localCplObjs[j])==theAddCpl[i].gid))
    {
      AddCoupling(context, localCplObjs[j], theAddCpl[i].proc, theAddCpl[i].prio);
    }
  }
}


/****************************************************************************/


static void CplMsgDisplay (DDD::DDDContext& context, const char *comment, LC_MSGHANDLE xm)
{
  using std::setw;

  std::ostream& out = std::cout;
  auto& ctx = context.cplmsgContext();
  TEDelCpl     *theDelCpl;
  TEModCpl     *theModCpl;
  TEAddCpl     *theAddCpl;
  int i, proc = LC_MsgGetProc(xm);
  int lenDelCpl = (int) LC_GetTableLen(xm, ctx.delcpl_id);
  int lenModCpl = (int) LC_GetTableLen(xm, ctx.modcpl_id);
  int lenAddCpl = (int) LC_GetTableLen(xm, ctx.addcpl_id);

  std::ostringstream prefixStream;
  prefixStream
    << " " << setw(3) << context.me() << "-" << comment << "-" << setw(3) << proc << " ";
  const std::string& prefix = prefixStream.str();

  /* get table addresses inside message */
  theDelCpl = (TEDelCpl *)    LC_GetPtr(xm, ctx.delcpl_id);
  theModCpl = (TEModCpl *)    LC_GetPtr(xm, ctx.modcpl_id);
  theAddCpl = (TEAddCpl *)    LC_GetPtr(xm, ctx.addcpl_id);


  out << prefix << " 04 DelCpl.size=" << setw(5) << lenDelCpl << "\n";
  out << prefix << " 05 ModCpl.size=" << setw(5) << lenModCpl << "\n";
  out << prefix << " 06 AddCpl.size=" << setw(5) << lenAddCpl << "\n";

  for(i=0; i<lenDelCpl; i++)
    out << prefix << " 14 delcpl " << setw(4) << i << " - "
        << theDelCpl[i].gid << "\n";

  for(i=0; i<lenModCpl; i++)
    out << prefix << " 15 modcpl " << setw(4) << i << " - "
        << theModCpl[i].gid << " " << setw(3) << theModCpl[i].prio << "\n";

  for(i=0; i<lenAddCpl; i++)
    out << prefix << " 16 addcpl " << setw(4) << i << " - "
        << theAddCpl[i].gid << " " << setw(4) << theAddCpl[i].proc
        << " " << setw(3) << theAddCpl[i].prio << "\n";
}


/****************************************************************************/


/*
        ...

        localCplObjs is a sorted list of all objects with coupling lists.
 */

void CommunicateCplMsgs (
  DDD::DDDContext& context,
  XIDelCpl **itemsDC, int nDC,
  XIModCpl **itemsMC, int nMC,
  XIAddCpl **itemsAC, int nAC,
  DDD_HDR *localCplObjs, int nLCO)
{
  auto& ctx = context.cplmsgContext();

  /* accumulate messages (one for each partner) */
  CplmsgList sendMsgs;
  int nSendMsgs;
  std::tie(nSendMsgs, sendMsgs) = PrepareCplMsgs(context,
                                                 itemsDC, nDC,
                                                 itemsMC, nMC,
                                                 itemsAC, nAC);

  /* init communication topology */
  int nRecvMsgs = LC_Connect(context, ctx.cplmsg_t);

  /* build and send messages */
  CplMsgSend(context, sendMsgs);


#if DebugCplMsg>2
  if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
#endif
  {
    for (const auto& msg : sendMsgs)
      CplMsgDisplay(context, "CS", msg.msg_h);
  }


  /* display information about send-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_XFER) & XFER_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD XFER_SHOW_MSGSALL: CplMsg.Send\n";
    LC_PrintSendMsgs(context);
  }


  /* communicate set of messages (send AND receive) */
  LC_MSGHANDLE* recvMsgs = LC_Communicate(context);


  /* display information about recv-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_XFER) & XFER_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD XFER_SHOW_MSGSALL: CplMsg.Recv\n";
    LC_PrintRecvMsgs(context);
  }


  for(int i=0; i<nRecvMsgs; i++)
  {
    CplMsgUnpackSingle(context, recvMsgs[i], localCplObjs, nLCO);

    /*
                    if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
                            CplMsgDisplay(context, "CR", recvMsgs[i]);
     */
  }


  /* cleanup low-comm layer */
  LC_Cleanup(context);
}


/****************************************************************************/

END_UGDIM_NAMESPACE
