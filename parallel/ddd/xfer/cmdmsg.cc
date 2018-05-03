// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      cmdmsg.c                                                      */
/*                                                                          */
/* Purpose:   ddd command transfer:                                         */
/*               send messages with commands to owners of other local       */
/*               object copies. this is used for sending XferCopy commands  */
/*               to owners of local copies, in order to prevent deletion    */
/*               and creation of the same object copy during xfer.          */
/*               (used only if OPT_XFER_PRUNE_DELETE is set to OPT_ON)      */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            internet: birken@ica3.uni-stuttgart.de                        */
/*                                                                          */
/* History:   970411 kb  created                                            */
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
#include <cassert>

#include <algorithm>
#include <forward_list>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include <dune/common/stdstreams.hh>

#include "dddi.h"
#include "xfer.h"



USING_UG_NAMESPACE
using namespace PPIF;

  START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

/* CMDMSG: complete description of message on sender side */

struct CMDMSG
{
  DDD_PROC proc;

  DDD_GID   *aUnDelete = nullptr;
  int nUnDelete = 0;

  /* lowcomm message handle */
  LC_MSGHANDLE msg_h;

  CMDMSG(DDD_PROC dest)
    : proc(dest)
    {}
};

using CmdmsgList = std::forward_list<CMDMSG>;


/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


void CmdMsgInit(DDD::DDDContext& context)
{
  auto& ctx = context.cmdmsgContext();

  ctx.cmdmsg_t = LC_NewMsgType(context, "CmdMsg");
  ctx.undelete_id = LC_NewMsgTable("UndelTab", ctx.cmdmsg_t, sizeof(DDD_GID));
}


void CmdMsgExit(DDD::DDDContext& context)
{}



/****************************************************************************/

static
std::pair<int, CmdmsgList>
PrepareCmdMsgs (DDD::DDDContext& context, const std::vector<XICopyObj*>& arrayCO)
{
  auto& ctx = context.cmdmsgContext();

  if (arrayCO.empty())
    return {0, CmdmsgList()};

#       if DebugCmdMsg<=3
  Dune::dvverb << "PreparePrune, nCopyObj=" << arrayCO.size() << "\n";
#       endif


  /*
          run through CopyObj table, mark all entries that have
          a coupling with same proc as destination.
   */

  int markedCO = 0;
  for(auto co : arrayCO)
  {
    DDD_PROC pCO = co->dest;
    COUPLING *cpl;

    /* run through coupling list of corresponding obj,
       find coupling to destination of XferCopyObj command */
    cpl = ObjCplList(context, co->hdr);
    while (cpl!=NULL && CPL_PROC(cpl)!=pCO)
      cpl=CPL_NEXT(cpl);

    if (cpl!=NULL)
    {
      /* found coupling -> mark CopyObj */
      SET_CO_SELF(co, 1);
      markedCO++;
    }
    else
    {
      SET_CO_SELF(co, 0);
    }
  }


  if (markedCO==0)
    return {0, CmdmsgList()};

  std::vector<DDD_GID> gids(markedCO);
  CmdmsgList msgs;
  int nMsgs = 0;

  /*
          run through CopyObj table again, consider only marked items,
          each time a new proc-nr is encountered in one of these
          tables, create a new CMDMSG item.

          (the lists have been sorted according to proc-nr previously.)
   */
  int j=0;
  for(auto co : arrayCO)
  {
    if (CO_SELF(co))
    {
      gids[j] = co->gid;

      if (msgs.empty() or msgs.front().proc != co->dest)
      {
        msgs.emplace_front(co->dest);
        nMsgs++;

        msgs.front().aUnDelete = &gids[j];
      }

      msgs.front().nUnDelete++;
      j++;
    }
  }

  /* initiate send messages */
  for(auto& xm : msgs)
  {
    /* create new send message */
    xm.msg_h = LC_NewSendMsg(context, ctx.cmdmsg_t, xm.proc);

    /* init tables inside message */
    LC_SetTableSize(xm.msg_h, ctx.undelete_id, xm.nUnDelete);

    /* prepare message for sending away */
    LC_MsgPrepareSend(context, xm.msg_h);

    DDD_GID* array = (DDD_GID *)LC_GetPtr(xm.msg_h, ctx.undelete_id);
    memcpy((char *)array,
           (char *)xm.aUnDelete,
           sizeof(DDD_GID)*xm.nUnDelete);
  }

  return {nMsgs, std::move(msgs)};
}


static void CmdMsgSend(DDD::DDDContext& context, const CmdmsgList& msgs)
{
  for(const auto& msg : msgs) {
    /* schedule message for send */
    LC_MsgSend(context, msg.msg_h);
  }
}



/****************************************************************************/



static int CmdMsgUnpack (DDD::DDDContext& context,
                         LC_MSGHANDLE *theMsgs, int nRecvMsgs,
                         XIDelCmd  **itemsDC, int nDC)
{
  auto& ctx = context.cmdmsgContext();

  int i, k, jDC, iDC, pos, nPruned;

  int lenGidTab = 0;
  for(i=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];
    lenGidTab += (int)LC_GetTableLen(xm, ctx.undelete_id);
  }

  if (lenGidTab==0)
    return(0);

  std::vector<DDD_GID> unionGidTab(lenGidTab);

  for(i=0, pos=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];
    int len = LC_GetTableLen(xm, ctx.undelete_id);

    if (len>0)
    {
      memcpy((char *) (unionGidTab.data()+pos),
             (char *)    LC_GetPtr(xm, ctx.undelete_id),
             sizeof(DDD_GID) * len);
      pos += len;
    }
  }
  assert(pos==lenGidTab);

  /* sort GidTab */
  std::sort(unionGidTab.begin(), unionGidTab.end());

        #ifdef SUPPORT_RESENT_FLAG
  {
    int iLCO, nLCO = context.couplingContext().nCpls;
    std::vector<DDD_HDR> localCplObjs = LocalCoupledObjectsList(context);

    /* set RESENT flag for objects which will receive another copy */
    iLCO=0;
    for(k=0; k<lenGidTab; k++)
    {
      while (iLCO<nLCO &&
             (OBJ_GID(localCplObjs[iLCO]) < unionGidTab[k]))
      {
        SET_OBJ_RESENT(localCplObjs[iLCO], 0);

#                               if DebugCmdMsg<=0
        Dune::dvverb << "PruneDelCmds. " << OBJ_GID(localCplObjs[iLCO]) << " without resent1.\n";
#                               endif

        iLCO++;
      }

      if (iLCO<nLCO && (OBJ_GID(localCplObjs[iLCO]) == unionGidTab[k]))
      {
        SET_OBJ_RESENT(localCplObjs[iLCO], 1);

#                               if DebugCmdMsg<=1
        Dune::dvverb << "PruneDelCmds. " << OBJ_GID(localCplObjs[iLCO]) << " will be resent.\n";
#                               endif

        iLCO++;
      }
    }

    /* reset remaining objects' flags */
    while (iLCO<nLCO)
    {
      SET_OBJ_RESENT(localCplObjs[iLCO], 0);

#                       if DebugCmdMsg<=0
      Dune::dvverb << "PruneDelCmds. " << OBJ_GID(localCplObjs[iLCO]) << " without resent2.\n";
#                       endif

      iLCO++;
    }
  }
        #endif


  k=0; jDC=0;
  for(iDC=0; iDC<nDC; iDC++)
  {
    DDD_GID gidDC = OBJ_GID(itemsDC[iDC]->hdr);

    while (k<lenGidTab && (unionGidTab[k]<gidDC))
      k++;

    if (k<lenGidTab && (unionGidTab[k]==gidDC))
    {
      /* found a DelCmd-item to prune */
      SET_OBJ_PRUNED(itemsDC[iDC]->hdr, 1);

#                       if DebugCmdMsg<=1
      Dune::dvverb << "PruneDelCmds. pruned " << gidDC << "\n";
#                       endif
    }
    else
    {
      itemsDC[jDC] = itemsDC[iDC];
      jDC++;
    }
  }
  nPruned = nDC-jDC;

#       if DebugCmdMsg<=3
  Dune::dvverb << "PruneDelCmds. nPruned=" << nPruned << "/" << nDC << "\n";
#       endif

  return(nPruned);
}


/****************************************************************************/


static void CmdMsgDisplay(DDD::DDDContext& context, const char *comment, LC_MSGHANDLE xm)
{
  auto& ctx = context.cmdmsgContext();

  using std::setw;
  std::ostream& out = std::cout;

  DDD_GID      *theGid;
  int proc = LC_MsgGetProc(xm);
  int lenGid = (int) LC_GetTableLen(xm, ctx.undelete_id);

  std::ostringstream prefixStream;
  prefixStream << setw(3) << context.me() << "-" << comment << setw(3) << proc << " ";
  const std::string& prefix = prefixStream.str();

  /* get table addresses inside message */
  theGid = (DDD_GID *)    LC_GetPtr(xm, ctx.undelete_id);

  out << prefix << " 04 Gid.size=" << setw(5) << lenGid << "\n";

  for(int i=0; i<lenGid; i++)
    out << prefix << " 14 gid    " << setw(4) << i << " - "
        << DDD_GID_TO_INT(theGid[i]) << "\n";
}


/****************************************************************************/


/*
        prune superfluous DelCmds. DelCmds are regarded superfluous,
        if there exists another processor which also owns an object copy
        and sends it to destination=me.

        this is implemented by sending the GID of all objects with an
        XferCopyObj-command with destination=p to p, iff p does already
        own a copy of the object. after receiving the messages,
        each processor prunes DelCmds for the GIDs in the message.

        returns:  number of pruned DelCmds.  ( < nDC)
 */

int PruneXIDelCmd (
  DDD::DDDContext& context,
  XIDelCmd  **itemsDC, int nDC,
  const std::vector<XICopyObj*>& arrayCO)
{
  auto& ctx = context.cmdmsgContext();

  /* accumulate messages (one for each partner) */
  int nSendMsgs;
  CmdmsgList sendMsgs;
  std::tie(nSendMsgs, sendMsgs) = PrepareCmdMsgs(context, arrayCO);

#if DebugCmdMsg>2
  if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
#endif
  {
    for (const auto& msg : sendMsgs)
      CmdMsgDisplay(context, "PS", msg.msg_h);
  }

  /* init communication topology */
  int nRecvMsgs = LC_Connect(context, ctx.cmdmsg_t);

  /* build and send messages */
  CmdMsgSend(context, sendMsgs);

  /* communicate set of messages (send AND receive) */
  LC_MSGHANDLE* recvMsgs = LC_Communicate(context);


  int nPruned = CmdMsgUnpack(context, recvMsgs, nRecvMsgs, itemsDC, nDC);


  /*
          for(int i=0; i<nRecvMsgs; i++)
          {
     #if DebugCmdMsg>=2
                  if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
     #endif
                          CmdMsgDisplay(context, "PR", recvMsgs[i]);
          }
   */


  /* cleanup low-comm layer */
  LC_Cleanup(context);

  return(nPruned);
}


/****************************************************************************/

END_UGDIM_NAMESPACE
