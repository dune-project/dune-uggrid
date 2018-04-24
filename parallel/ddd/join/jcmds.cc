// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      jcmds.c                                                       */
/*                                                                          */
/* Purpose:   DDD-commands for Join Environment                             */
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
/* History:   980126 kb  begin                                              */
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

#include <algorithm>
#include <iomanip>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include "dddi.h"
#include "join.h"


USING_UG_NAMESPACE
using namespace PPIF;

START_UGDIM_NAMESPACE

using namespace DDD::Join;

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/





/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
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


/*
        prepare messages for phase 1.

 */

static int PreparePhase1Msgs (DDD::DDDContext& context, std::vector<JIJoin*>& arrayJoin,
                              JOINMSG1 **theMsgs, size_t *memUsage)
{
  auto& ctx = context.joinContext();

  int i, last_i, nMsgs;
  JIJoin** itemsJ = arrayJoin.data();
  const int nJ = arrayJoin.size();

#       if DebugJoin<=3
  printf("%4d: PreparePhase1Msgs, nJoins=%d\n",
         me, nJ);
  fflush(stdout);
#       endif

  /* init return parameters */
  *theMsgs = NULL;
  *memUsage = 0;


  if (nJ==0)
    /* no messages */
    return(0);


  /* check whether Join objects are really local (without copies) */
  /* and set local GID to invalid (will be set to new value lateron) */
  for(i=0; i<nJ; i++)
  {
    if (ObjHasCpl(context, itemsJ[i]->hdr))
      DUNE_THROW(Dune::Exception,
                 "cannot join " << OBJ_GID(itemsJ[i]->hdr)
                 << ", object already distributed");

    OBJ_GID(itemsJ[i]->hdr) = GID_INVALID;
  }


  /* set local GID to new value */
  for(i=0; i<nJ; i++)
  {
    DDD_GID local_gid = OBJ_GID(itemsJ[i]->hdr);

    /* check for double Joins with different new_gid */
    if (local_gid!=GID_INVALID && local_gid!=itemsJ[i]->new_gid)
      DUNE_THROW(Dune::Exception,
                 "several (inconsistent) DDD_JoinObj-commands "
                 "for local object " << local_gid);

    OBJ_GID(itemsJ[i]->hdr) = itemsJ[i]->new_gid;
  }


  nMsgs = 0;
  last_i = i = 0;
  do
  {
    size_t bufSize;

    /* skip until dest-processor is different */
    while (i<nJ && (itemsJ[i]->dest == itemsJ[last_i]->dest))
      i++;

    /* create new message */
    JOINMSG1* jm = new JOINMSG1;
    jm->nJoins = i-last_i;
    jm->arrayJoin = &(itemsJ[last_i]);
    jm->dest = itemsJ[last_i]->dest;
    jm->next = *theMsgs;
    *theMsgs = jm;
    nMsgs++;

    /* create new send message */
    jm->msg_h = LC_NewSendMsg(context, ctx.phase1msg_t, jm->dest);

    /* init table inside message */
    LC_SetTableSize(jm->msg_h, ctx.jointab_id, jm->nJoins);

    /* prepare message for sending away */
    bufSize = LC_MsgPrepareSend(context, jm->msg_h);
    *memUsage += bufSize;

    if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MEMUSAGE)
    {
      Dune::dwarn
        << "DDD MESG [" << std::setw(3) << me << "]: SHOW_MEM "
        << "send msg phase1   dest=" << std::setw(4) << jm->dest
        << " size=" << std::setw(10) << bufSize << "\n";
    }

    last_i = i;

  } while (last_i < nJ);

  return(nMsgs);
}



/****************************************************************************/
/*                                                                          */
/* Function:  PackPhase1Msgs                                                */
/*                                                                          */
/* Purpose:   allocate one message buffer for each outgoing message,        */
/*            fill buffer with message contents and initiate asynchronous   */
/*            send for each message.                                        */
/*                                                                          */
/* Input:     theMsgs: list of message-send-infos                           */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

static void PackPhase1Msgs (DDD::DDDContext& context, JOINMSG1 *theMsgs)
{
  auto& ctx = context.joinContext();
  JOINMSG1 *jm;

  for(jm=theMsgs; jm!=NULL; jm=jm->next)
  {
    TEJoin *theJoinTab;
    int i;

    /* copy data into message */
    theJoinTab = (TEJoin *)LC_GetPtr(jm->msg_h, ctx.jointab_id);
    for(i=0; i<jm->nJoins; i++)
    {
      theJoinTab[i].gid  = jm->arrayJoin[i]->new_gid;
      theJoinTab[i].prio = OBJ_PRIO(jm->arrayJoin[i]->hdr);
    }
    LC_SetTableLen(jm->msg_h, ctx.jointab_id, jm->nJoins);


    /* send away */
    LC_MsgSend(context, jm->msg_h);
  }
}



/*
        unpack phase1 messages.
 */

static void UnpackPhase1Msgs (DDD::DDDContext& context,
                              LC_MSGHANDLE *theMsgs, int nRecvMsgs,
                              DDD_HDR *localCplObjs, int nLCO,
                              JIPartner **p_joinObjs, int *p_nJoinObjs)
{
  auto& ctx = context.joinContext();
  JIPartner *joinObjs;
  int nJoinObjs = 0;
  int m, jo;

  /* init return values */
  *p_joinObjs  = NULL;
  *p_nJoinObjs = 0;


  for(m=0; m<nRecvMsgs; m++)
  {
    LC_MSGHANDLE jm = theMsgs[m];
    TEJoin *theJoin = (TEJoin *) LC_GetPtr(jm, ctx.jointab_id);
    int nJ       = (int) LC_GetTableLen(jm, ctx.jointab_id);
    int i, j;

    nJoinObjs += nJ;

    for(i=0, j=0; i<nJ; i++)
    {
      while ((j<nLCO) && (OBJ_GID(localCplObjs[j]) < theJoin[i].gid))
        j++;

      if ((j<nLCO) && (OBJ_GID(localCplObjs[j])==theJoin[i].gid))
      {
        COUPLING *cpl;

        /* found local object which is join target */
        /* store shortcut to local object */
        theJoin[i].hdr = localCplObjs[j];

        /* generate phase2-JIAddCpl for this object */
        for(cpl=ObjCplList(context, localCplObjs[j]); cpl!=NULL; cpl=CPL_NEXT(cpl))
        {
          JIAddCpl *ji = JIAddCplSet_NewItem(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl2));
          ji->dest    = CPL_PROC(cpl);
          ji->te.gid  = theJoin[i].gid;
          ji->te.proc = LC_MsgGetProc(jm);
          ji->te.prio = theJoin[i].prio;

          if (! JIAddCplSet_ItemOK(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl2)))
            continue;

#                                       if DebugJoin<=1
          printf("%4d: Phase1 Join for " DDD_GID_FMT " from %d, "
                 "send AddCpl to %d.\n",
                 me, theJoin[i].gid, ji->te.proc, ji->dest);
#                                       endif

        }

        /* send phase3-JIAddCpl back to Join-proc */
        for(cpl=ObjCplList(context, localCplObjs[j]); cpl!=NULL; cpl=CPL_NEXT(cpl))
        {
          JIAddCpl *ji = JIAddCplSet_NewItem(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));
          ji->dest    = LC_MsgGetProc(jm);
          ji->te.gid  = OBJ_GID(localCplObjs[j]);
          ji->te.proc = CPL_PROC(cpl);
          ji->te.prio = cpl->prio;

          if (! JIAddCplSet_ItemOK(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3)))
            continue;
        }
      }
      else
      {
        DUNE_THROW(Dune::Exception,
                   "no object " << theJoin[i].gid
                   << " for join from " << LC_MsgGetProc(jm));
      }
    }
  }


  /* return immediately if no join-objects have been found */
  if (nJoinObjs==0)
    return;


  /* allocate array of objects, which has been contacted by a join */
  joinObjs = new JIPartner[nJoinObjs];

  /* set return values */
  *p_joinObjs  = joinObjs;
  *p_nJoinObjs = nJoinObjs;


  /* add one local coupling for each Join */
  for(m=0, jo=0; m<nRecvMsgs; m++)
  {
    LC_MSGHANDLE jm = theMsgs[m];
    TEJoin *theJoin = (TEJoin *) LC_GetPtr(jm, ctx.jointab_id);
    int nJ       = (int) LC_GetTableLen(jm, ctx.jointab_id);
    int i;

    for(i=0; i<nJ; i++)
    {
      AddCoupling(context, theJoin[i].hdr, LC_MsgGetProc(jm), theJoin[i].prio);

      /* send one phase3-JIAddCpl for symmetric connection */
      {
        JIAddCpl *ji = JIAddCplSet_NewItem(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));
        ji->dest    = LC_MsgGetProc(jm);
        ji->te.gid  = OBJ_GID(theJoin[i].hdr);
        ji->te.proc = me;
        ji->te.prio = OBJ_PRIO(theJoin[i].hdr);

        JIAddCplSet_ItemOK(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));
      }

      joinObjs[jo].hdr  = theJoin[i].hdr;
      joinObjs[jo].proc = LC_MsgGetProc(jm);
      jo++;
    }
  }


  /* sort joinObjs-array according to gid */
  if (nJoinObjs>1) {
    std::sort(
      joinObjs, joinObjs + nJoinObjs,
      [](const JIPartner& a, const JIPartner& b) {
        return OBJ_GID(a.hdr) < OBJ_GID(b.hdr);
      });
  }
}



/****************************************************************************/

/*
        prepare messages for phase 2.

 */

static int PreparePhase2Msgs (DDD::DDDContext& context, std::vector<JIAddCpl*>& arrayAddCpl,
                              JOINMSG2 **theMsgs, size_t *memUsage)
{
  auto& ctx = context.joinContext();

  int i, last_i, nMsgs;
  JIAddCpl** itemsAC = arrayAddCpl.data();
  const int nAC = arrayAddCpl.size();

#       if DebugJoin<=3
  printf("%4d: PreparePhase2Msgs, nAddCpls=%d\n",
         me, nAC);
  fflush(stdout);
#       endif

  /* init return parameters */
  *theMsgs = NULL;
  *memUsage = 0;


  if (nAC==0)
    /* no messages */
    return(0);


  nMsgs = 0;
  last_i = i = 0;
  do
  {
    size_t bufSize;

    /* skip until dest-processor is different */
    while (i<nAC && (itemsAC[i]->dest == itemsAC[last_i]->dest))
      i++;

    /* create new message */
    JOINMSG2* jm = new JOINMSG2;
    jm->nAddCpls = i-last_i;
    jm->arrayAddCpl = &(itemsAC[last_i]);
    jm->dest = itemsAC[last_i]->dest;
    jm->next = *theMsgs;
    *theMsgs = jm;
    nMsgs++;

    /* create new send message */
    jm->msg_h = LC_NewSendMsg(context, ctx.phase2msg_t, jm->dest);

    /* init table inside message */
    LC_SetTableSize(jm->msg_h, ctx.addtab_id, jm->nAddCpls);

    /* prepare message for sending away */
    bufSize = LC_MsgPrepareSend(context, jm->msg_h);
    *memUsage += bufSize;

    if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MEMUSAGE)
    {
      Dune::dwarn
        << "DDD MESG [" << std::setw(3) << me << "]: SHOW_MEM "
        << "send msg phase2   dest=" << std::setw(4) << jm->dest
        << " size=" << std::setw(10) << bufSize << "\n";
    }

    last_i = i;

  } while (last_i < nAC);

  return(nMsgs);
}



/****************************************************************************/
/*                                                                          */
/* Function:  PackPhase2Msgs                                                */
/*                                                                          */
/* Purpose:   allocate one message buffer for each outgoing message,        */
/*            fill buffer with message contents and initiate asynchronous   */
/*            send for each message.                                        */
/*                                                                          */
/* Input:     theMsgs: list of message-send-infos                           */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

static void PackPhase2Msgs(DDD::DDDContext& context, JOINMSG2 *theMsgs)
{
  auto& ctx = context.joinContext();

  JOINMSG2 *jm;

  for(jm=theMsgs; jm!=NULL; jm=jm->next)
  {
    TEAddCpl *theAddTab;
    int i;

    /* copy data into message */
    theAddTab = (TEAddCpl *)LC_GetPtr(jm->msg_h, ctx.addtab_id);
    for(i=0; i<jm->nAddCpls; i++)
    {
      /* copy complete TEAddCpl item */
      theAddTab[i] = jm->arrayAddCpl[i]->te;
    }
    LC_SetTableLen(jm->msg_h, ctx.addtab_id, jm->nAddCpls);


    /* send away */
    LC_MsgSend(context, jm->msg_h);
  }
}



/*
        unpack phase2 messages.
 */

static void UnpackPhase2Msgs (DDD::DDDContext& context,
                              LC_MSGHANDLE *theMsgs2, int nRecvMsgs2,
                              JIPartner *joinObjs, int nJoinObjs,
                              DDD_HDR *localCplObjs, int nLCO)
{
  auto& ctx = context.joinContext();

  int m;

  for(m=0; m<nRecvMsgs2; m++)
  {
    LC_MSGHANDLE jm = theMsgs2[m];
    TEAddCpl *theAC = (TEAddCpl *) LC_GetPtr(jm, ctx.addtab_id);
    int nAC      = (int) LC_GetTableLen(jm, ctx.addtab_id);
    int i, j, jo;

    for(i=0, j=0, jo=0; i<nAC; i++)
    {
      while ((j<nLCO) && (OBJ_GID(localCplObjs[j]) < theAC[i].gid))
        j++;

      while ((jo<nJoinObjs) && (OBJ_GID(joinObjs[jo].hdr) < theAC[i].gid))
        jo++;

      if ((j<nLCO) && (OBJ_GID(localCplObjs[j])==theAC[i].gid))
      {
        /* found local object which is AddCpl target */
        AddCoupling(context, localCplObjs[j], theAC[i].proc, theAC[i].prio);

#                               if DebugJoin<=1
        printf("%4d: Phase2 execute AddCpl(%08x,%d,%d) (from %d).\n",
               me,
               theAC[i].gid, theAC[i].proc, theAC[i].prio,
               LC_MsgGetProc(jm));
#                               endif

        while ((jo<nJoinObjs) && (OBJ_GID(joinObjs[jo].hdr) == theAC[i].gid))
        {
          JIAddCpl *ji = JIAddCplSet_NewItem(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));
          ji->dest    = joinObjs[jo].proc;
          ji->te.gid  = theAC[i].gid;
          ji->te.proc = theAC[i].proc;
          ji->te.prio = theAC[i].prio;
          JIAddCplSet_ItemOK(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));


#                                       if DebugJoin<=1
          printf("%4d: Phase2 forward AddCpl(%08x,%d,%d) to %d.\n",
                 me,
                 theAC[i].gid, theAC[i].proc, theAC[i].prio,
                 ji->dest);
#                                       endif

          jo++;
        }
      }
      else
      {
        /* this should never happen. AddCpl send from invalid proc. */
        assert(0);
      }
    }
  }
}



/****************************************************************************/



/*
        prepare messages for phase 3.

 */

static int PreparePhase3Msgs (DDD::DDDContext& context, std::vector<JIAddCpl*>& arrayAddCpl,
                              JOINMSG3 **theMsgs, size_t *memUsage)
{
  auto& ctx = context.joinContext();

  int i, last_i, nMsgs;
  JIAddCpl** itemsAC = arrayAddCpl.data();
  const int nAC = arrayAddCpl.size();

#       if DebugJoin<=3
  printf("%4d: PreparePhase3Msgs, nAddCpls=%d\n",
         me, nAC);
  fflush(stdout);
#       endif

  /* init return parameters */
  *theMsgs = NULL;
  *memUsage = 0;


  if (nAC==0)
    /* no messages */
    return(0);


  nMsgs = 0;
  last_i = i = 0;
  do
  {
    size_t bufSize;

    /* skip until dest-processor is different */
    while (i<nAC && (itemsAC[i]->dest == itemsAC[last_i]->dest))
      i++;

    /* create new message */
    JOINMSG3* jm = new JOINMSG3;
    jm->nAddCpls = i-last_i;
    jm->arrayAddCpl = &(itemsAC[last_i]);
    jm->dest = itemsAC[last_i]->dest;
    jm->next = *theMsgs;
    *theMsgs = jm;
    nMsgs++;

    /* create new send message */
    jm->msg_h = LC_NewSendMsg(context, ctx.phase3msg_t, jm->dest);

    /* init table inside message */
    LC_SetTableSize(jm->msg_h, ctx.cpltab_id, jm->nAddCpls);

    /* prepare message for sending away */
    bufSize = LC_MsgPrepareSend(context, jm->msg_h);
    *memUsage += bufSize;

    if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MEMUSAGE)
    {
      Dune::dwarn
        << "DDD MESG [" << std::setw(3) << me << "]: SHOW_MEM "
        << "send msg phase3   dest=" << std::setw(4) << jm->dest
        << " size=" << std::setw(10) << bufSize << "\n";
    }

    last_i = i;

  } while (last_i < nAC);

  return(nMsgs);
}



/****************************************************************************/
/*                                                                          */
/* Function:  PackPhase3Msgs                                                */
/*                                                                          */
/* Purpose:   allocate one message buffer for each outgoing message,        */
/*            fill buffer with message contents and initiate asynchronous   */
/*            send for each message.                                        */
/*                                                                          */
/* Input:     theMsgs: list of message-send-infos                           */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

static void PackPhase3Msgs(DDD::DDDContext& context, JOINMSG3 *theMsgs)
{
  auto& ctx = context.joinContext();

  JOINMSG3 *jm;

  for(jm=theMsgs; jm!=NULL; jm=jm->next)
  {
    TEAddCpl *theAddTab;
    int i;

    /* copy data into message */
    theAddTab = (TEAddCpl *)LC_GetPtr(jm->msg_h, ctx.cpltab_id);
    for(i=0; i<jm->nAddCpls; i++)
    {
      /* copy complete TEAddCpl item */
      theAddTab[i] = jm->arrayAddCpl[i]->te;
    }
    LC_SetTableLen(jm->msg_h, ctx.cpltab_id, jm->nAddCpls);


    /* send away */
    LC_MsgSend(context, jm->msg_h);
  }
}



/****************************************************************************/


/*
        unpack phase3 messages.
 */

static void UnpackPhase3Msgs (DDD::DDDContext& context,
                              LC_MSGHANDLE *theMsgs, int nRecvMsgs,
                              std::vector<JIJoin*>& arrayJoin)
{
  auto& ctx = context.joinContext();

  JIJoin** itemsJ = arrayJoin.data();
  const int nJ = arrayJoin.size();
  int m;


  for(m=0; m<nRecvMsgs; m++)
  {
    LC_MSGHANDLE jm = theMsgs[m];
    TEAddCpl *theAC = (TEAddCpl *) LC_GetPtr(jm, ctx.cpltab_id);
    int nAC      = (int) LC_GetTableLen(jm, ctx.cpltab_id);
    int i, j;

    for(i=0, j=0; i<nAC; i++)
    {
      while ((j<nJ) && (OBJ_GID(itemsJ[j]->hdr) < theAC[i].gid))
        j++;

      if ((j<nJ) && (OBJ_GID(itemsJ[j]->hdr) == theAC[i].gid))
      {
        /* found local object which is AddCpl target */
        AddCoupling(context, itemsJ[j]->hdr, theAC[i].proc, theAC[i].prio);

#                               if DebugJoin<=1
        printf("%4d: Phase3 execute AddCpl(%08x,%d,%d) (from %d).\n",
               me,
               OBJ_GID(itemsJ[j]->hdr), theAC[i].proc, theAC[i].prio,
               LC_MsgGetProc(jm));
#                               endif
      }
      else
      {
        /* this should never happen. AddCpl send for unknown obj. */
        assert(0);
      }
    }
  }
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_JoinEnd                                                   */
/*                                                                          */
/****************************************************************************/

/**
        End of join phase.
        This function starts the actual join process. After a call to
        this function (on all processors) all {\bf Join}-commands since
        the last call to \funk{JoinBegin} are executed. This involves
        a set of local communications between the processors.
 */

DDD_RET DDD_JoinEnd(DDD::DDDContext& context)
{
  auto& ctx = context.joinContext();

  int obsolete, nRecvMsgs1, nRecvMsgs2, nRecvMsgs3;
  JOINMSG1    *sendMsgs1=NULL, *sm1=NULL;
  JOINMSG2    *sendMsgs2=NULL, *sm2=NULL;
  JOINMSG3    *sendMsgs3=NULL, *sm3=NULL;
  LC_MSGHANDLE *recvMsgs1=NULL, *recvMsgs2=NULL, *recvMsgs3=NULL;
  size_t sendMem=0, recvMem=0;
  JIPartner   *joinObjs = NULL;
  int nJoinObjs;
  const auto& nCpls = context.couplingContext().nCpls;



        #ifdef JoinMemFromHeap
  MarkHeap();
  LC_SetMemMgr(context,
               memmgr_AllocTMEM, memmgr_FreeTMEM,
               memmgr_AllocHMEM, NULL);
        #endif

  STAT_SET_MODULE(DDD_MODULE_JOIN);
  STAT_ZEROALL;

  /* step mode and check whether call to JoinEnd is valid */
  if (!JoinStepMode(context, JoinMode::JMODE_CMDS))
    DUNE_THROW(Dune::Exception, "DDD_JoinEnd() aborted");


  /*
          PREPARATION PHASE
   */
  /* get sorted array of JIJoin-items */
  std::vector<JIJoin*> arrayJIJoin = JIJoinSet_GetArray(reinterpret_cast<JIJoinSet*>(ctx.setJIJoin));
  obsolete = JIJoinSet_GetNDiscarded(reinterpret_cast<JIJoinSet*>(ctx.setJIJoin));


  /*
          COMMUNICATION PHASE 1
          all processors, where JoinObj-commands have been issued,
          send information about these commands to the target
          processors together with the GID of the objects on the
          target procs and the local priority.
   */
  STAT_RESET;
  /* prepare msgs for JIJoin-items */
  PreparePhase1Msgs(context, arrayJIJoin, &sendMsgs1, &sendMem);
  /* DisplayMemResources(); */

  /* init communication topology */
  nRecvMsgs1 = LC_Connect(context, ctx.phase1msg_t);
  STAT_TIMER(T_JOIN_PREP_MSGS);

  STAT_RESET;
  /* build phase1 msgs on sender side and start send */
  PackPhase1Msgs(context, sendMsgs1);
  STAT_TIMER(T_JOIN_PACK_SEND);


  /*
          now messages are in the net, use spare time
   */
  STAT_RESET;
  /* get sorted list of local objects with couplings */
  std::vector<DDD_HDR> localCplObjs = LocalCoupledObjectsList(context);

  if (obsolete>0)
  {
    if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_OBSOLETE)
    {
      int all = JIJoinSet_GetNItems(reinterpret_cast<JIJoinSet*>(ctx.setJIJoin));

      using std::setw;
      Dune::dwarn
        << "DDD MESG [" << setw(3) << me << "]: " << setw(4) << obsolete
        << " from " << setw(4) << all << " join-cmds obsolete.\n";
    }
  }
  STAT_TIMER(T_JOIN);


  /*
          nothing more to do until incoming messages arrive
   */

  /* display information about send-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD JOIN_SHOW_MSGSALL: Phase1Msg.Send\n";
    LC_PrintSendMsgs(context);
  }


  /* wait for communication-completion (send AND receive) */
  STAT_RESET;
  recvMsgs1 = LC_Communicate(context);
  STAT_TIMER(T_JOIN_WAIT_RECV);


  /* display information about message buffer sizes */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MEMUSAGE)
  {
    int k;

    /* sum up sizes of receive mesg buffers */
    for(k=0; k<nRecvMsgs1; k++)
    {
      recvMem += LC_GetBufferSize(recvMsgs1[k]);
    }

    using std::setw;
    Dune::dwarn
      << "DDD MESG [" << setw(3) << me << "]: SHOW_MEM msgs "
      << " send=" << setw(10) << sendMem
      << " recv=" << setw(10) << recvMem
      << " all=" << setw(10) << (sendMem+recvMem) << "\n";
  }

  /* display information about recv-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD JOIN_SHOW_MSGSALL: Phase1Msg.Recv\n";
    LC_PrintRecvMsgs(context);
  }

  /* unpack messages */
  STAT_RESET;
  UnpackPhase1Msgs(context, recvMsgs1, nRecvMsgs1, localCplObjs.data(), nCpls,
                   &joinObjs, &nJoinObjs);
  LC_Cleanup(context);
  STAT_TIMER(T_JOIN_UNPACK);





  /*
          COMMUNICATION PHASE 2
          all processors which received notification of JoinObj-commands
          during phase 1 send AddCpl-requests to all copies of DDD objects,
          for which Joins had been issued remotely.
   */
  /* get sorted array of JIAddCpl-items */
  std::vector<JIAddCpl*> arrayJIAddCpl2 = JIAddCplSet_GetArray(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl2));

  STAT_RESET;
  /* prepare msgs for JIAddCpl-items */
  PreparePhase2Msgs(context, arrayJIAddCpl2, &sendMsgs2, &sendMem);
  /* DisplayMemResources(); */

  /* init communication topology */
  nRecvMsgs2 = LC_Connect(context, ctx.phase2msg_t);
  STAT_TIMER(T_JOIN_PREP_MSGS);

  STAT_RESET;
  /* build phase2 msgs on sender side and start send */
  PackPhase2Msgs(context, sendMsgs2);
  STAT_TIMER(T_JOIN_PACK_SEND);

  /*
          now messages are in the net, use spare time
   */

  /* reorder Join-commands according to new_gid */
  /* this ordering is needed in UnpackPhase3 */
  if (arrayJIJoin.size() > 1)
  {
    std::sort(
      arrayJIJoin.begin(), arrayJIJoin.end(),
      [](const JIJoin* a, const JIJoin* b) {
        return a->new_gid < b->new_gid;
      });
  }


  /*
          nothing more to do until incoming messages arrive
   */

  /* display information about send-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn <<"DDD JOIN_SHOW_MSGSALL: Phase2Msg.Send\n";
    LC_PrintSendMsgs(context);
  }


  /* wait for communication-completion (send AND receive) */
  STAT_RESET;
  recvMsgs2 = LC_Communicate(context);
  STAT_TIMER(T_JOIN_WAIT_RECV);


  /* display information about message buffer sizes */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MEMUSAGE)
  {
    int k;

    /* sum up sizes of receive mesg buffers */
    for(k=0; k<nRecvMsgs2; k++)
    {
      recvMem += LC_GetBufferSize(recvMsgs2[k]);
    }

    using std::setw;
    Dune::dwarn
      << "DDD MESG [" << setw(3) << me << "]: SHOW_MEM msgs "
      << " send=" << setw(10) << sendMem
      << " recv=" << setw(10) << recvMem
      << " all=" << setw(10) << (sendMem+recvMem) << "\n";
  }

  /* display information about recv-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD JOIN_SHOW_MSGSALL: Phase2Msg.Recv\n";
    LC_PrintRecvMsgs(context);
  }

  /* unpack messages */
  STAT_RESET;
  UnpackPhase2Msgs(context, recvMsgs2, nRecvMsgs2, joinObjs, nJoinObjs,
                   localCplObjs.data(), nCpls);

  LC_Cleanup(context);
  STAT_TIMER(T_JOIN_UNPACK);

  for(; sendMsgs2!=NULL; sendMsgs2=sm2)
  {
    sm2 = sendMsgs2->next;
    delete sendMsgs2;
  }






  /*
          COMMUNICATION PHASE 3
          all processors which received notification of JoinObj-commands
          during phase 1 send AddCpl-requests to the procs where the
          JoinObj-commands have been issued. One AddCpl-request is sent
          for each cpl in the local objects coupling list. One AddCpl-request
          is sent for each AddCpl-request received during phase 2.
          (i.e., two kinds of AddCpl-requests are send to the processors
          on which the JoinObj-commands have been issued.
   */
  /* get sorted array of JIAddCpl-items */
  std::vector<JIAddCpl*> arrayJIAddCpl3 = JIAddCplSet_GetArray(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));

  STAT_RESET;
  /* prepare msgs for JIAddCpl-items */
  PreparePhase3Msgs(context, arrayJIAddCpl3, &sendMsgs3, &sendMem);
  /* DisplayMemResources(); */

  /* init communication topology */
  nRecvMsgs3 = LC_Connect(context, ctx.phase3msg_t);
  STAT_TIMER(T_JOIN_PREP_MSGS);

  STAT_RESET;
  /* build phase3 msgs on sender side and start send */
  PackPhase3Msgs(context, sendMsgs3);
  STAT_TIMER(T_JOIN_PACK_SEND);

  /*
          now messages are in the net, use spare time
   */
  /* ... */

  /*
          nothing more to do until incoming messages arrive
   */

  /* display information about send-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD JOIN_SHOW_MSGSALL: Phase3Msg.Send\n";
    LC_PrintSendMsgs(context);
  }


  /* wait for communication-completion (send AND receive) */
  STAT_RESET;
  recvMsgs3 = LC_Communicate(context);
  STAT_TIMER(T_JOIN_WAIT_RECV);


  /* display information about message buffer sizes */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MEMUSAGE)
  {
    int k;

    /* sum up sizes of receive mesg buffers */
    for(k=0; k<nRecvMsgs3; k++)
    {
      recvMem += LC_GetBufferSize(recvMsgs3[k]);
    }

    using std::setw;
    Dune::dwarn
      << "DDD MESG [" << setw(3) << me << "]: SHOW_MEM msgs "
      << " send=" << setw(10) << sendMem
      << " recv=" << setw(10) << recvMem
      << " all=" << setw(10) << (sendMem+recvMem) << "\n";
  }

  /* display information about recv-messages on lowcomm-level */
  if (DDD_GetOption(context, OPT_INFO_JOIN) & JOIN_SHOW_MSGSALL)
  {
    DDD_SyncAll(context);
    if (context.isMaster())
      Dune::dwarn << "DDD JOIN_SHOW_MSGSALL: Phase3Msg.Recv\n";
    LC_PrintRecvMsgs(context);
  }

  /* unpack messages */
  STAT_RESET;
  UnpackPhase3Msgs(context, recvMsgs3, nRecvMsgs3, arrayJIJoin);
  LC_Cleanup(context);
  STAT_TIMER(T_JOIN_UNPACK);

  for(; sendMsgs3!=NULL; sendMsgs3=sm3)
  {
    sm3 = sendMsgs3->next;
    delete sendMsgs3;
  }






  /*
          free temporary storage
   */
  JIJoinSet_Reset(reinterpret_cast<JIJoinSet*>(ctx.setJIJoin));

  JIAddCplSet_Reset(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl2));

  JIAddCplSet_Reset(reinterpret_cast<JIAddCplSet*>(ctx.setJIAddCpl3));

  if (joinObjs!=NULL)
    delete[] joinObjs;

  for(; sendMsgs1!=NULL; sendMsgs1=sm1)
  {
    sm1 = sendMsgs1->next;
    delete sendMsgs1;
  }



        #ifdef JoinMemFromHeap
  ReleaseHeap();
  LC_SetMemMgr(context,
               memmgr_AllocTMEM, memmgr_FreeTMEM,
               memmgr_AllocTMEM, memmgr_FreeTMEM);
        #endif

#       if DebugJoin<=4
  Dune::dverb << "JoinEnd, before IFAllFromScratch().\n";
#       endif

  /* re-create all interfaces and step JMODE */
  STAT_RESET;
  IFAllFromScratch(context);
  STAT_TIMER(T_JOIN_BUILD_IF);


  JoinStepMode(context, JoinMode::JMODE_BUSY);

  return(DDD_RET_OK);
}





/****************************************************************************/
/*                                                                          */
/* Function:  DDD_JoinObj                                                   */
/*                                                                          */
/****************************************************************************/

/**
        Join local object with a distributed object.

        \todoTBC

   @param hdr  DDD local object which should be joined.
 */



void DDD_JoinObj(DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC dest, DDD_GID new_gid)
{
  auto& ctx = context.joinContext();
  const auto procs = context.procs();

  if (!ddd_JoinActive(context))
    DUNE_THROW(Dune::Exception, "Missing DDD_JoinBegin()");

  if (dest>=procs)
    DUNE_THROW(Dune::Exception,
               "cannot join " << OBJ_GID(hdr) << " with " << new_gid
               << " on processor " << dest << " (procs=" << procs << ")");

  if (dest==context.me())
    DUNE_THROW(Dune::Exception,
               "cannot join " << OBJ_GID(hdr) << " with myself");

  if (ObjHasCpl(context, hdr))
    DUNE_THROW(Dune::Exception,
               "cannot join " << OBJ_GID(hdr)
               << ", object already distributed");



  JIJoin* ji = JIJoinSet_NewItem(reinterpret_cast<JIJoinSet*>(ctx.setJIJoin));
  ji->hdr     = hdr;
  ji->dest    = dest;
  ji->new_gid = new_gid;

  if (! JIJoinSet_ItemOK(reinterpret_cast<JIJoinSet*>(ctx.setJIJoin)))
    return;

#       if DebugJoin<=2
  Dune:dvverb << "DDD_JoinObj " << OBJ_GID(hdr)
              << ", dest=" << dest << ", new_gid=" << new_gid << "\n";
#       endif
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_JoinBegin                                                 */
/*                                                                          */
/****************************************************************************/

/**
        Starts join phase.
        A call to this function establishes a global join operation.
        It must be issued on all processors. After this call an arbitrary
        series of {\bf Join}-commands may be issued. The global transfer operation
        is carried out via a \funk{JoinEnd} call on each processor.
 */

void DDD_JoinBegin(DDD::DDDContext& context)
{
  /* step mode and check whether call to JoinBegin is valid */
  if (!JoinStepMode(context, JoinMode::JMODE_IDLE))
    DUNE_THROW(Dune::Exception, "DDD_JoinBegin() aborted");
}



END_UGDIM_NAMESPACE
