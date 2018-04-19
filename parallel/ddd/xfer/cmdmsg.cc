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

  CMDMSG *next;


  DDD_GID   *aUnDelete;
  int nUnDelete;

  /* lowcomm message handle */
  LC_MSGHANDLE msg_h;
};


/****************************************************************************/
/*                                                                          */
/* variables global to this source file only (static)                       */
/*                                                                          */
/****************************************************************************/





static LC_MSGTYPE cmdmsg_t;
static LC_MSGCOMP undelete_id;



/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


void CmdMsgInit(DDD::DDDContext& context)
{
  cmdmsg_t = LC_NewMsgType(context, "CmdMsg");
  undelete_id = LC_NewMsgTable("UndelTab", cmdmsg_t, sizeof(DDD_GID));
}


void CmdMsgExit(DDD::DDDContext& context)
{}



/****************************************************************************/

static CMDMSG *CreateCmdMsg (DDD_PROC dest, CMDMSG *lastxm)
{
  CMDMSG *xm;

  xm = (CMDMSG *) AllocTmp(sizeof(CMDMSG));
  if (xm==NULL)
    throw std::bad_alloc();

  xm->aUnDelete = NULL;
  xm->nUnDelete = 0;
  xm->proc = dest;
  xm->next = lastxm;

  return(xm);
}



static int PrepareCmdMsgs (DDD::DDDContext& context, XICopyObj **itemsCO, int nCO, CMDMSG **theMsgs)
{
  CMDMSG    *xm=NULL;
  int j, iCO, markedCO, nMsgs=0;
  DDD_GID   *gids;


  /* set output value in case of early exit */
  *theMsgs = NULL;


  if (nCO==0)
    return(0);

#       if DebugCmdMsg<=3
  Dune::dvverb << "PreparePrune, nCopyObj=" << nCO << "\n";
#       endif


  /*
          run through CopyObj table, mark all entries that have
          a coupling with same proc as destination.
   */

  markedCO = 0;
  for(iCO=0; iCO<nCO; iCO++)
  {
    XICopyObj *co = itemsCO[iCO];
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
    return(0);


  gids = (DDD_GID *) AllocTmp(sizeof(DDD_GID) * markedCO);
  if (gids==NULL)
    throw std::bad_alloc();


  /*
          run through CopyObj table again, consider only marked items,
          each time a new proc-nr is encountered in one of these
          tables, create a new CMDMSG item.

          (the lists have been sorted according to proc-nr previously.)
   */
  j=0;
  for(iCO=0; iCO<nCO; iCO++)
  {
    XICopyObj *co = itemsCO[iCO];

    if (CO_SELF(co))
    {
      gids[j] = co->gid;

      if ((xm==NULL) || (xm->proc!=co->dest))
      {
        xm = CreateCmdMsg(co->dest, xm);
        nMsgs++;


        xm->aUnDelete = gids+j;
      }

      xm->nUnDelete++;
      j++;
    }
  }
  *theMsgs = xm;


  /* initiate send messages */
  for(xm=*theMsgs; xm!=NULL; xm=xm->next)
  {
    DDD_GID *array;

    /* create new send message */
    xm->msg_h = LC_NewSendMsg(context, cmdmsg_t, xm->proc);

    /* init tables inside message */
    LC_SetTableSize(xm->msg_h, undelete_id, xm->nUnDelete);

    /* prepare message for sending away */
    LC_MsgPrepareSend(context, xm->msg_h);

    array = (DDD_GID *)LC_GetPtr(xm->msg_h, undelete_id);
    memcpy((char *)array,
           (char *)xm->aUnDelete,
           sizeof(DDD_GID)*xm->nUnDelete);
  }

  FreeTmp(gids,0);

  return(nMsgs);
}


static void CmdMsgSend(DDD::DDDContext& context, CMDMSG *theMsgs)
{
  CMDMSG *m;

  for(m=theMsgs; m!=NULL; m=m->next)
  {
    /* schedule message for send */
    LC_MsgSend(context, m->msg_h);
  }
}



/****************************************************************************/



static int CmdMsgUnpack (DDD::DDDContext& context,
                         LC_MSGHANDLE *theMsgs, int nRecvMsgs,
                         XIDelCmd  **itemsDC, int nDC)
{
  int i, k, jDC, iDC, pos, nPruned;

  int lenGidTab = 0;
  for(i=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];
    lenGidTab += (int)LC_GetTableLen(xm, undelete_id);
  }

  if (lenGidTab==0)
    return(0);

  std::vector<DDD_GID> unionGidTab(lenGidTab);

  for(i=0, pos=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];
    int len = LC_GetTableLen(xm, undelete_id);

    if (len>0)
    {
      memcpy((char *) (unionGidTab.data()+pos),
             (char *)    LC_GetPtr(xm, undelete_id),
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


static void CmdMsgDisplay (const char *comment, LC_MSGHANDLE xm)
{
  using std::setw;
  std::ostream& out = std::cout;

  DDD_GID      *theGid;
  char buf[30];
  int proc = LC_MsgGetProc(xm);
  int lenGid = (int) LC_GetTableLen(xm, undelete_id);

  std::ostringstream prefixStream;
  prefixStream << setw(3) << me << "-" << comment << setw(3) << proc << " ";
  const std::string& prefix = prefixStream.str();

  /* get table addresses inside message */
  theGid = (DDD_GID *)    LC_GetPtr(xm, undelete_id);

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
  std::vector<XICopyObj*>& arrayCO)
{
  CMDMSG    *sendMsgs, *sm=0;
  LC_MSGHANDLE *recvMsgs;
  int nSendMsgs, nRecvMsgs;
  int nPruned;

  XICopyObj** itemsCO = arrayCO.data();
  const int nCO = arrayCO.size();

  /* accumulate messages (one for each partner) */
  nSendMsgs = PrepareCmdMsgs(context, itemsCO, nCO, &sendMsgs);

#if DebugCmdMsg>2
  if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
#endif
  {
    for(sm=sendMsgs; sm!=NULL; sm=sm->next)
    {
      CmdMsgDisplay("PS", sm->msg_h);
    }
  }

  /* init communication topology */
  nRecvMsgs = LC_Connect(context, cmdmsg_t);

  /* build and send messages */
  CmdMsgSend(context, sendMsgs);

  /* communicate set of messages (send AND receive) */
  recvMsgs = LC_Communicate(context);


  nPruned = CmdMsgUnpack(context, recvMsgs, nRecvMsgs, itemsDC, nDC);


  /*
          for(int i=0; i<nRecvMsgs; i++)
          {
     #if DebugCmdMsg>=2
                  if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
     #endif
                          CmdMsgDisplay("PR", recvMsgs[i]);
          }
   */


  /* cleanup low-comm layer */
  LC_Cleanup(context);


  /* free temporary memory */
  for(; sendMsgs!=NULL; sendMsgs=sm)
  {
    sm = sendMsgs->next;
    FreeTmp(sendMsgs,0);
  }

  return(nPruned);
}


/****************************************************************************/

END_UGDIM_NAMESPACE
