// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      cons.c                                                        */
/*                                                                          */
/* Purpose:   consistency checker for ddd structures                        */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/03/21 kb  begin                                            */
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
#include <cassert>

#include <algorithm>
#include <vector>

#include <dune/uggrid/parallel/ddd/dddi.h>
#include <dune/uggrid/parallel/ddd/basic/lowcomm.h>

#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

  START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

/*
   PAIRS:     check existance of object for each coupling
   ALLTOALL:  check if all coupling lists are equal
 */
#define CHECK_CPL_PAIRS
#define CHECK_CPL_ALLTOALL



/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

struct CONS_INFO
{
  DDD_GID gid;
  DDD_TYPE typ;
  DDD_PROC dest;
  DDD_PROC proc;
  DDD_PRIO prio;
};




struct CONSMSG
{
  DDD_PROC dest;

  CONSMSG *next;


  CONS_INFO *consArray;
  int nItems;

  /* lowcomm message handle */
  LC_MSGHANDLE msg_h;
};



/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


void ddd_ConsInit(DDD::DDDContext& context)
{
  auto& ctx = context.consContext();
  ctx.consmsg_t = LC_NewMsgType(context, "ConsCheckMsg");
  ctx.constab_id = LC_NewMsgTable("ConsTab", ctx.consmsg_t, sizeof(CONS_INFO));
}


void ddd_ConsExit(DDD::DDDContext&)
{}


/****************************************************************************/

static int ConsBuildMsgInfos(DDD::DDDContext& context, CONS_INFO *allItems, int nXferItems, CONSMSG **theMsgs)
{
  CONSMSG    *cm, *lastCm;
  int i, lastdest, nMsgs;

  auto& ctx = context.consContext();

  lastdest = -1;
  lastCm = cm = NULL;
  nMsgs = 0;
  for(i=0; i<nXferItems; i++)
  {
    /* create new message item, if necessary */
    if (allItems[i].dest != lastdest)
    {
      cm = (CONSMSG *) AllocTmpReq(sizeof(CONSMSG), TMEM_CONS);
      if (cm==NULL)
      {
        DDD_PrintError('E', 9900, STR_NOMEM " in ConsBuildMsgInfos");
        return(0);
      }

      cm->nItems     = 0;
      cm->consArray = &allItems[i];
      cm->dest = allItems[i].dest;
      cm->next = lastCm;
      lastCm = cm;
      lastdest = cm->dest;
      nMsgs++;
    }
    cm->nItems++;
  }
  *theMsgs = cm;


  /* initiate send messages */
  for(cm=*theMsgs; cm!=NULL; cm=cm->next)
  {
    /* create new send message */
    cm->msg_h = LC_NewSendMsg(context, ctx.consmsg_t, cm->dest);

    /* init table inside message */
    LC_SetTableSize(cm->msg_h, ctx.constab_id, cm->nItems);

    /* prepare message for sending away */
    LC_MsgPrepareSend(context, cm->msg_h);
  }

  return(nMsgs);
}



static void ConsSend(DDD::DDDContext& context, CONSMSG *theMsgs)
{
  auto& ctx = context.consContext();

  for(CONSMSG* cm=theMsgs; cm != nullptr; cm=cm->next)
  {
    /* copy data into message */
    memcpy(LC_GetPtr(cm->msg_h, ctx.constab_id),
           cm->consArray, sizeof(CONS_INFO)*cm->nItems);

    /* send message */
    LC_MsgSend(context, cm->msg_h);
  }
}



static int ConsCheckSingleMsg (DDD::DDDContext& context, LC_MSGHANDLE xm, DDD_HDR *locObjs)
{
  CONS_INFO    *theCplBuf;
  int i, j, nItems;
  int error_cnt = 0;

  auto& ctx = context.consContext();
  const auto& me = context.me();

  nItems = (int) LC_GetTableLen(xm, ctx.constab_id);
  theCplBuf = (CONS_INFO *) LC_GetPtr(xm, ctx.constab_id);


  /*
          sprintf(cBuffer, "%4d: checking message from proc %d (%d items)\n",
                  me, LC_MsgGetProc(xm), nItems);
          DDD_PrintDebug(cBuffer);
   */

  const int nObjs = context.nObjs();

  /* test whether there are consistent objects for all couplings */
  for(i=0, j=0; i<nItems; i++)
  {
    while ((j < nObjs) && (OBJ_GID(locObjs[j]) < theCplBuf[i].gid))
      j++;

    if ((j < nObjs) && (OBJ_GID(locObjs[j])==theCplBuf[i].gid))
    {
      if (OBJ_PRIO(locObjs[j])!=theCplBuf[i].prio)
      {
        Dune::dwarn
          << "    DDD-CCC Warning: obj " << OBJ_GID(locObjs[j]) << " type "
          << OBJ_TYPE(locObjs[j]) << " on " << me << " has prio "
          << OBJ_PRIO(locObjs[j]) << ", cpl from " << LC_MsgGetProc(xm)
          << " has prio " << theCplBuf[i].prio << "!\n";

        error_cnt++;
      }
    }
    else
    {
      Dune::dwarn
        << "    DDD-GCC Warning: obj " << theCplBuf[i].gid << " type "
        << theCplBuf[i].typ << " on " << me << " for cpl from "
        << LC_MsgGetProc(xm) << "missing!\n";

      error_cnt++;
    }
  }

  return(error_cnt);
}



static bool sort_CplBufDest (const CONS_INFO& a, const CONS_INFO& b)
{
  return std::tie(a.dest, a.gid) < std::tie(b.dest, b.gid);
}


static int ConsCheckGlobalCpl(DDD::DDDContext& context)
{
  COUPLING     *cpl;
  int i, j, lenCplBuf, nRecvMsgs;
  CONSMSG      *sendMsgs=NULL, *cm=NULL;
  LC_MSGHANDLE *recvMsgs;
  int error_cnt = 0;

  auto& ctx = context.consContext();
  const auto procs = context.procs();
  const auto& objTable = context.objTable();

  /* count overall number of couplings */
  const auto& nCpls = context.couplingContext().nCpls;
  for(i=0, lenCplBuf=0; i < nCpls; i++)
    lenCplBuf += IdxNCpl(context, i);

  /* get storage for messages */
  std::vector<CONS_INFO> cplBuf(lenCplBuf);

  /* copy CONS_INFOs into message buffer */
  for(i=0, j=0; i < nCpls; i++)
  {
    for(cpl=IdxCplList(context, i); cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      if ((DDD_PROC)CPL_PROC(cpl) >= procs)
      {
        error_cnt++;
        Dune::dwarn << "DDD-GCC Warning: invalid proc=" << CPL_PROC(cpl)
                    << " (" << OBJ_GID(cpl->obj) << "/" << OBJ_GID(objTable[i]) << ")\n";
      }
      cplBuf[j].gid  = OBJ_GID(cpl->obj);
      cplBuf[j].typ  = OBJ_TYPE(cpl->obj);
      cplBuf[j].dest = CPL_PROC(cpl);
      cplBuf[j].proc = CPL_PROC(cpl);
      cplBuf[j].prio = cpl->prio;
      j++;
    }
  }
  assert(j==lenCplBuf);

  /* sort couplings */
  std::sort(cplBuf.begin(), cplBuf.end(), sort_CplBufDest);

  /* accumulate messages (one for each partner); inform receivers */
  ConsBuildMsgInfos(context, cplBuf.data(), cplBuf.size(), &sendMsgs);

  /* init communication topology */
  nRecvMsgs = LC_Connect(context, ctx.consmsg_t);
  if (nRecvMsgs==ERROR)
  {
    error_cnt = -1;
    goto exit_ConsCheckGlobalCpl;
  }

  /* build and send messages */
  ConsSend(context, sendMsgs);


  /* communicate set of messages (send AND receive) */
  recvMsgs = LC_Communicate(context);


  /* perform checking of received data */
  if (nRecvMsgs>0)
  {
    std::vector<DDD_HDR> locObjs = LocalObjectsList(context);

    for(i=0; i<nRecvMsgs; i++)
    {
      error_cnt += ConsCheckSingleMsg(context, recvMsgs[i], locObjs.data());
    }
  }



exit_ConsCheckGlobalCpl:

  /* cleanup low-comm layer */
  LC_Cleanup(context);


  /* free temporary storage */
  for(; sendMsgs!=NULL; sendMsgs=cm)
  {
    cm = sendMsgs->next;
    FreeTmpReq(sendMsgs, sizeof(CONSMSG), TMEM_CONS);
  }

  return(error_cnt);
}




/****************************************************************************/


static int Cons2CheckSingleMsg (DDD::DDDContext& context, LC_MSGHANDLE xm, DDD_HDR *locObjs)
{
  CONS_INFO    *theCplBuf;
  int i, inext=0, j, nItems;
  int error_cnt = 0;

  auto& ctx = context.consContext();
  const auto& me = context.me();

  nItems = (int) LC_GetTableLen(xm, ctx.constab_id);
  theCplBuf = (CONS_INFO *) LC_GetPtr(xm, ctx.constab_id);


  /*
          sprintf(cBuffer, "%4d: checking message from proc %d (%d items)\n",
                  me, LC_MsgGetProc(xm), nItems);
          DDD_PrintDebug(cBuffer);
   */

  const int nObjs = context.nObjs();

  /* test whether there are consistent objects for all couplings */
  for(i=0, j=0; i<nItems; i=inext)
  {
    inext = i+1;

    while ((j < nObjs) && (OBJ_GID(locObjs[j]) < theCplBuf[i].gid))
      j++;

    if ((j < nObjs) && (OBJ_GID(locObjs[j])==theCplBuf[i].gid))
    {
      if (theCplBuf[i].proc == me)
      {
        if (OBJ_PRIO(locObjs[j])!=theCplBuf[i].prio)
        {
          Dune::dwarn
            << "    DDD-GCC Warning: obj " << OBJ_GID(locObjs[j]) << " type "
            << OBJ_TYPE(locObjs[j]) << " on " << me << " has prio "
            << OBJ_PRIO(locObjs[j]) << ", cpl from " << LC_MsgGetProc(xm)
            << " has prio " << theCplBuf[i].prio << "!\n";

          error_cnt++;
        }
      }
      else
      {
        int i2;
        COUPLING *j2;

        for(j2=ObjCplList(context, locObjs[j]); j2!=NULL; j2=CPL_NEXT(j2))
        {
          int ifound = -1;

          for(i2=i; i2<nItems && theCplBuf[i2].gid==theCplBuf[i].gid;
              i2++)
          {
            if (theCplBuf[i2].proc==CPL_PROC(j2))
            {
              ifound = i2;
              break;
            }
          }

          if (ifound==-1)
          {
            Dune::dwarn
              << "    DDD-GCC Warning: obj " << theCplBuf[i].gid << " type "
              << theCplBuf[i].typ << " on " << me << " has cpl from "
              << CPL_PROC(j2) << ", but " << LC_MsgGetProc(xm) << " hasn't!\n";

            error_cnt++;
          }
        }


        /* the next loop does the backward checking, i.e., it checks
                the couplings symmetric to the previous loop. if inconsistencies
                are detected, then these inconsistencies are a local phenomenon
                and can be "healed" by adding a coupling locally. this feature
                is switched off, because this healing indicates that the
                data structure is inconsistent, which should be repaired
                elsewhere (in the DDD application).
         */
                                #ifdef ConsCheckWithAutomaticHealing

        for(i2=i; i2<nItems && theCplBuf[i2].gid==theCplBuf[i].gid; i2++)
        {
          if (theCplBuf[i2].proc!=me)
          {
            int ifound = -1;

            for(j2=ObjCplList(context, locObjs[j]); j2!=NULL; j2=CPL_NEXT(j2))
            {
              if (theCplBuf[i2].proc==j2->proc)
              {
                ifound = i2;
                break;
              }
            }

            if (ifound==-1)
            {
              Dune::dwarn
                << "healing with AddCpl(" << theCplBuf[i].gid << ", "
                << theCplBuf[i2].proc << ", " << theCplBuf[i2].prio << ")\n";

              AddCoupling(context, locObjs[j], theCplBuf[i2].proc, theCplBuf[i2].prio);
            }
          }
        }
                                #endif /* ConsCheckWithAutomaticHealing */


        for(; inext<nItems && theCplBuf[inext].gid==theCplBuf[i].gid; inext++)
          ;
      }
    }
    /*   this is superfluous and (more important:) wrong!
                    else
                    {
                            sprintf(cBuffer, " X  DDD-GCC Warning: obj %08x type %d on %d for cpl"
                                    " from %3d missing!\n",
                                    theCplBuf[i].gid, theCplBuf[i].typ, me, LC_MsgGetProc(xm));
                            DDD_PrintLine(cBuffer);

                            error_cnt++;
                    }
     */
  }

  /* the next too lines are wrong, bug find by PURIFY. KB 970416.
     the locObjs list is freed in Cons2CheckGlobalCpl!
     if (locObjs!=NULL)
          FreeTmp(locObjs,0);
   */


  return(error_cnt);
}



static int Cons2CheckGlobalCpl(DDD::DDDContext& context)
{
  COUPLING     *cpl, *cpl2;
  int i, j, lenCplBuf, nRecvMsgs;
  CONSMSG      *sendMsgs, *cm=0;
  LC_MSGHANDLE *recvMsgs;
  int error_cnt = 0;

  auto& ctx = context.consContext();
  const auto& me = context.me();
  const auto& nCpls = context.couplingContext().nCpls;

  /* count overall number of couplings */
  for(i=0, lenCplBuf=0; i < nCpls; i++)
    lenCplBuf += (IdxNCpl(context, i) * (IdxNCpl(context, i)+1));

  /* get storage for messages */
  std::vector<CONS_INFO> cplBuf(lenCplBuf);

  /* copy CONS_INFOs into message buffer */
  for(i=0, j=0; i < nCpls; i++)
  {
    for(cpl=IdxCplList(context, i); cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      cplBuf[j].gid  = OBJ_GID(cpl->obj);
      cplBuf[j].typ  = OBJ_TYPE(cpl->obj);
      cplBuf[j].dest = CPL_PROC(cpl);
      cplBuf[j].proc = me;
      cplBuf[j].prio = OBJ_PRIO(cpl->obj);
      j++;

      for(cpl2=IdxCplList(context, i); cpl2!=NULL; cpl2=CPL_NEXT(cpl2))
      {
        cplBuf[j].gid  = OBJ_GID(cpl->obj);
        cplBuf[j].typ  = OBJ_TYPE(cpl->obj);
        cplBuf[j].dest = CPL_PROC(cpl);
        cplBuf[j].proc = CPL_PROC(cpl2);
        cplBuf[j].prio = cpl2->prio;
        j++;
      }
    }
  }
  assert(j==lenCplBuf);

  /* sort couplings */
  std::sort(cplBuf.begin(), cplBuf.end(), sort_CplBufDest);

  /* accumulate messages (one for each partner); inform receivers */
  ConsBuildMsgInfos(context, cplBuf.data(), cplBuf.size(), &sendMsgs);

  /* init communication topology */
  nRecvMsgs = LC_Connect(context, ctx.consmsg_t);

  /* build and send messages */
  ConsSend(context, sendMsgs);

  /* communicate set of messages (send AND receive) */
  recvMsgs = LC_Communicate(context);


  /* perform checking of received data */
  if (nRecvMsgs>0)
  {
    std::vector<DDD_HDR> locObjs = LocalObjectsList(context);

    for(i=0; i<nRecvMsgs; i++) {
      error_cnt += Cons2CheckSingleMsg(context, recvMsgs[i], locObjs.data());
    }
  }


  /* cleanup low-comm layer */
  LC_Cleanup(context);


  /* free temporary storage */
  for(; sendMsgs!=NULL; sendMsgs=cm)
  {
    cm = sendMsgs->next;
    FreeTmpReq(sendMsgs, sizeof(CONSMSG), TMEM_CONS);
  }

  return(error_cnt);
}



/****************************************************************************/

static int ConsCheckDoubleObj(const DDD::DDDContext& context)
{
  std::vector<DDD_HDR> locObjs = LocalObjectsList(context);
  const int nObjs = context.nObjs();

  int error_cnt = 0;
  for(int i=1; i < nObjs; i++)
  {
    if (OBJ_GID(locObjs[i-1])==OBJ_GID(locObjs[i]))
    {
      error_cnt++;
      Dune::dwarn << "    DDD-GCC Warning: obj " << OBJ_GID(locObjs[i])
                  << " on " << context.me() << " doubled\n";
    }
  }

  return(error_cnt);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_ConsCheck                                                 */
/*                                                                          */
/****************************************************************************/

/**
        Check DDD runtime consistency.
        This function performs a combined local/global consistency
        check on the object data structures and interfaces managed by DDD.
        This may be used for debugging purposes; if errors are detected,
        then some understanding of internal DDD structures will be useful.

        The following single aspects will be checked:
        \begin{itemize}
        \item double existence of {\em global ID} numbers in each processor's
                  set of local objects.
        \item consistency of coupling lists and object copies
        \item non-symmetric interfaces between processor pairs
        \item non-symmetric number of items in each interface
        \end{itemize}

   @returns  total number of errors (sum of all procs)
 */

int DDD_ConsCheck(DDD::DDDContext& context)
{
  int cpl_errors;
  int total_errors=0;

  DDD_Flush();
  Synchronize(context.ppifContext());
  if (DDD_GetOption(context, OPT_QUIET_CONSCHECK)==OPT_OFF)
  {
    if (context.isMaster())
      DDD_PrintLine("   DDD-GCC (Global Consistency Check)\n");
  }

  total_errors += ConsCheckDoubleObj(context);

        #ifdef CHECK_CPL_PAIRS
  cpl_errors = ConsCheckGlobalCpl(context);
        #endif
        #ifdef CHECK_CPL_ALLTOALL
  cpl_errors = Cons2CheckGlobalCpl(context);
        #endif

  if (cpl_errors==-1)
  {
    Dune::dgrave << "    DDD-GCC Error: out of memory in ConsCheckGlobalCpl()\n";
    total_errors++;
  }
  else
    total_errors += cpl_errors;

  total_errors += DDD_CheckInterfaces(context);


  /* compute sum of errors over all processors */
  total_errors = ddd_GlobalSumInt(context, total_errors);

  DDD_Flush();
  Synchronize(context.ppifContext());
  if (DDD_GetOption(context, OPT_QUIET_CONSCHECK)==OPT_OFF)
  {
    if (context.isMaster())
      Dune::dwarn << "   DDD-GCC ready (" << total_errors << " errors)\n";
  }


  return(total_errors);
}


END_UGDIM_NAMESPACE
