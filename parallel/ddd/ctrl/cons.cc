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

#include "dddi.h"
/*#include "xfer/xfer.h"*/
#include "basic/lowcomm.h"

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

typedef struct
{
  DDD_GID gid;
  DDD_TYPE typ;
  DDD_PROC dest;
  DDD_PROC proc;
  DDD_PRIO prio;
} CONS_INFO;




typedef struct _CONSMSG
{
  DDD_PROC dest;

  struct _CONSMSG *next;


  CONS_INFO *consArray;
  int nItems;

  /* lowcomm message handle */
  LC_MSGHANDLE msg_h;

} CONSMSG;



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




static LC_MSGTYPE consmsg_t;
static LC_MSGCOMP constab_id;


#ifdef ConsMemFromHeap
static long theMarkKey;
#endif



/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


void ddd_ConsInit(DDD::DDDContext& context)
{
  consmsg_t = LC_NewMsgType(context, "ConsCheckMsg");
  constab_id = LC_NewMsgTable("ConsTab", consmsg_t, sizeof(CONS_INFO));
}


void ddd_ConsExit(DDD::DDDContext&)
{}


#ifdef ConsMemFromHeap
static void *cons_AllocHeap (size_t size)
{
  void *buffer = AllocHeap(size, theMarkKey);
  return(buffer);
}

static void *cons_AllocSend (size_t size)
{
  void *buffer = AllocTmpReq(size, TMEM_ANY);
  return(buffer);
}

static void cons_FreeSend (void *buffer)
{
  FreeTmpReq(buffer, 0, TMEM_ANY);
}
#endif


/****************************************************************************/

static int ConsBuildMsgInfos(DDD::DDDContext& context, CONS_INFO *allItems, int nXferItems, CONSMSG **theMsgs)
{
  CONSMSG    *cm, *lastCm;
  int i, lastdest, nMsgs;

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
    cm->msg_h = LC_NewSendMsg(context, consmsg_t, cm->dest);

    /* init table inside message */
    LC_SetTableSize(cm->msg_h, constab_id, cm->nItems);

    /* prepare message for sending away */
    LC_MsgPrepareSend(context, cm->msg_h);
  }

  return(nMsgs);
}



static void ConsSend(DDD::DDDContext& context, CONSMSG *theMsgs)
{
  CONSMSG *cm;

  for(cm=theMsgs; cm!=NULL; cm=cm->next)
  {
    /* copy data into message */
    memcpy(LC_GetPtr(cm->msg_h, constab_id),
           cm->consArray, sizeof(CONS_INFO)*cm->nItems);

    /* send message */
    LC_MsgSend(context, cm->msg_h);
  }
}



static int ConsCheckSingleMsg (LC_MSGHANDLE xm, DDD_HDR *locObjs)
{
  CONS_INFO    *theCplBuf;
  int i, j, nItems;
  int error_cnt = 0;


  nItems = (int) LC_GetTableLen(xm,constab_id);
  theCplBuf = (CONS_INFO *) LC_GetPtr(xm, constab_id);


  /*
          sprintf(cBuffer, "%4d: checking message from proc %d (%d items)\n",
                  me, LC_MsgGetProc(xm), nItems);
          DDD_PrintDebug(cBuffer);
   */


  /* test whether there are consistent objects for all couplings */
  for(i=0, j=0; i<nItems; i++)
  {
    while ((j<ddd_nObjs) && (OBJ_GID(locObjs[j]) < theCplBuf[i].gid))
      j++;

    if ((j<ddd_nObjs) && (OBJ_GID(locObjs[j])==theCplBuf[i].gid))
    {
      if (OBJ_PRIO(locObjs[j])!=theCplBuf[i].prio)
      {
        sprintf(cBuffer, "    DDD-GCC Warning: obj " OBJ_GID_FMT " type %d on %d"
                " has prio %d, cpl from %d has prio %d!\n",
                OBJ_GID(locObjs[j]), OBJ_TYPE(locObjs[j]), me, OBJ_PRIO(locObjs[j]),
                LC_MsgGetProc(xm), theCplBuf[i].prio);
        DDD_PrintLine(cBuffer);

        error_cnt++;
      }
    }
    else
    {
      sprintf(cBuffer, "    DDD-GCC Warning: obj " DDD_GID_FMT " type %d on %d for cpl"
              " from %3d missing!\n",
              theCplBuf[i].gid, theCplBuf[i].typ, me, LC_MsgGetProc(xm));
      DDD_PrintLine(cBuffer);

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
  DDD_HDR      *locObjs = NULL;

  const auto procs = context.procs();

  /* count overall number of couplings */
  for(i=0, lenCplBuf=0; i<NCpl_Get; i++)
    lenCplBuf += IdxNCpl(i);

  /* get storage for messages */
  std::vector<CONS_INFO> cplBuf(lenCplBuf);

  /* copy CONS_INFOs into message buffer */
  for(i=0, j=0; i<NCpl_Get; i++)
  {
    for(cpl=IdxCplList(i); cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      if ((DDD_PROC)CPL_PROC(cpl) >= procs)
      {
        error_cnt++;
        sprintf(cBuffer, "%4d: DDD-GCC Warning: invalid proc=%d (" OBJ_GID_FMT "/" OBJ_GID_FMT ")\n",
                me, CPL_PROC(cpl), OBJ_GID(cpl->obj),
                OBJ_GID(ddd_ObjTable[i])
                );
        DDD_PrintLine(cBuffer);
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
  nRecvMsgs = LC_Connect(context, consmsg_t);
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
    locObjs = LocalObjectsList();
    if (locObjs==NULL && ddd_nObjs>0)
    {
      DDD_PrintLine(
        "    DDD-GCC Warning: out of memory in ConsCheckGlobalCpl()\n");
      error_cnt++;                   /* one additional error */
      goto exit_ConsCheckGlobalCpl;
    }

    for(i=0; i<nRecvMsgs; i++)
    {
      error_cnt += ConsCheckSingleMsg(recvMsgs[i], locObjs);
    }
    FreeLocalObjectsList(locObjs);
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


static int Cons2CheckSingleMsg (LC_MSGHANDLE xm, DDD_HDR *locObjs)
{
  CONS_INFO    *theCplBuf;
  int i, inext=0, j, nItems;
  int error_cnt = 0;


  nItems = (int) LC_GetTableLen(xm,constab_id);
  theCplBuf = (CONS_INFO *) LC_GetPtr(xm, constab_id);


  /*
          sprintf(cBuffer, "%4d: checking message from proc %d (%d items)\n",
                  me, LC_MsgGetProc(xm), nItems);
          DDD_PrintDebug(cBuffer);
   */


  /* test whether there are consistent objects for all couplings */
  for(i=0, j=0; i<nItems; i=inext)
  {
    inext = i+1;

    while ((j<ddd_nObjs) && (OBJ_GID(locObjs[j]) < theCplBuf[i].gid))
      j++;

    if ((j<ddd_nObjs) && (OBJ_GID(locObjs[j])==theCplBuf[i].gid))
    {
      if (theCplBuf[i].proc == me)
      {
        if (OBJ_PRIO(locObjs[j])!=theCplBuf[i].prio)
        {
          sprintf(cBuffer, "    DDD-GCC Warning: obj " OBJ_GID_FMT " type %d on %d"
                  " has prio %d, cpl from %d has prio %d!\n",
                  OBJ_GID(locObjs[j]), OBJ_TYPE(locObjs[j]), me, OBJ_PRIO(locObjs[j]),
                  LC_MsgGetProc(xm), theCplBuf[i].prio);
          DDD_PrintLine(cBuffer);

          error_cnt++;
        }
      }
      else
      {
        int i2;
        COUPLING *j2;

        for(j2=ObjCplList(locObjs[j]); j2!=NULL; j2=CPL_NEXT(j2))
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
            sprintf(cBuffer, "    DDD-GCC Warning: obj " DDD_GID_FMT " type %d on %d has cpl"
                    " from%4d, but %d hasn't!\n",
                    theCplBuf[i].gid, theCplBuf[i].typ, me,
                    CPL_PROC(j2), LC_MsgGetProc(xm));
            DDD_PrintLine(cBuffer);

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

            for(j2=ObjCplList(locObjs[j]); j2!=NULL; j2=CPL_NEXT(j2))
            {
              if (theCplBuf[i2].proc==j2->proc)
              {
                ifound = i2;
                break;
              }
            }

            if (ifound==-1)
            {
              sprintf(cBuffer, "%4d: healing with AddCpl(%08x, %4d, %d)\n",
                      me, theCplBuf[i].gid, theCplBuf[i2].proc, theCplBuf[i2].prio);
              DDD_PrintLine(cBuffer);

              AddCoupling(locObjs[j], theCplBuf[i2].proc, theCplBuf[i2].prio);
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
  DDD_HDR      *locObjs = NULL;

  /* count overall number of couplings */
  for(i=0, lenCplBuf=0; i<NCpl_Get; i++)
    lenCplBuf += (IdxNCpl(i) * (IdxNCpl(i)+1));

  /* get storage for messages */
  std::vector<CONS_INFO> cplBuf(lenCplBuf);

  /* copy CONS_INFOs into message buffer */
  for(i=0, j=0; i<NCpl_Get; i++)
  {
    for(cpl=IdxCplList(i); cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      cplBuf[j].gid  = OBJ_GID(cpl->obj);
      cplBuf[j].typ  = OBJ_TYPE(cpl->obj);
      cplBuf[j].dest = CPL_PROC(cpl);
      cplBuf[j].proc = me;
      cplBuf[j].prio = OBJ_PRIO(cpl->obj);
      j++;

      for(cpl2=IdxCplList(i); cpl2!=NULL; cpl2=CPL_NEXT(cpl2))
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
  nRecvMsgs = LC_Connect(context, consmsg_t);

  /* build and send messages */
  ConsSend(context, sendMsgs);

  /* communicate set of messages (send AND receive) */
  recvMsgs = LC_Communicate(context);


  /* perform checking of received data */
  if (nRecvMsgs>0)
  {
    locObjs = LocalObjectsList();
    if (locObjs==NULL && ddd_nObjs>0)
    {
      DDD_PrintLine(
        "    DDD-GCC Warning: out of memory in Cons2CheckGlobalCpl()\n");
      error_cnt++;                   /* one additional error */
    }
    else
    {
      for(i=0; i<nRecvMsgs; i++)
      {
        error_cnt += Cons2CheckSingleMsg(recvMsgs[i], locObjs);
      }
      FreeLocalObjectsList(locObjs);
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

static int ConsCheckDoubleObj (void)
{
  DDD_HDR      *locObjs;
  int i, error_cnt = 0;

  locObjs = LocalObjectsList();
  if (locObjs==NULL && ddd_nObjs>0)
  {
    DDD_PrintLine("    DDD-GCC Warning: out of memory in ConsCheckDoubleObj()\n");
    return(1);             /* report one error */
  }

  for(i=1; i<ddd_nObjs; i++)
  {
    if (OBJ_GID(locObjs[i-1])==OBJ_GID(locObjs[i]))
    {
      error_cnt++;
      sprintf(cBuffer, "    DDD-GCC Warning: obj " OBJ_GID_FMT " on %d doubled\n",
              OBJ_GID(locObjs[i]), me);
      DDD_PrintLine(cBuffer);
    }
  }

  FreeLocalObjectsList(locObjs);

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

        #ifdef ConsMemFromHeap
  MarkHeap(&theMarkKey);
  LC_SetMemMgrRecv(context, cons_AllocHeap, NULL);
  LC_SetMemMgrSend(context, cons_AllocSend, cons_FreeSend);
        #endif

  DDD_Flush();
  Synchronize(context.ppifContext());
  if (DDD_GetOption(OPT_QUIET_CONSCHECK)==OPT_OFF)
  {
    if (context.isMaster())
      DDD_PrintLine("   DDD-GCC (Global Consistency Check)\n");
  }

  total_errors += ConsCheckDoubleObj();

        #ifdef CHECK_CPL_PAIRS
  cpl_errors = ConsCheckGlobalCpl(context);
        #endif
        #ifdef CHECK_CPL_ALLTOALL
  cpl_errors = Cons2CheckGlobalCpl(context);
        #endif

  if (cpl_errors==-1)
  {
    DDD_PrintLine("    DDD-GCC Error: out of memory in ConsCheckGlobalCpl()\n");
    total_errors++;
  }
  else
    total_errors += cpl_errors;

  total_errors += DDD_CheckInterfaces(context);


  /* compute sum of errors over all processors */
  total_errors = ddd_GlobalSumInt(context, total_errors);

  DDD_Flush();
  Synchronize(context.ppifContext());
  if (DDD_GetOption(OPT_QUIET_CONSCHECK)==OPT_OFF)
  {
    if (context.isMaster())
    {
      sprintf(cBuffer, "   DDD-GCC ready (%d errors)\n", total_errors);
      DDD_PrintLine(cBuffer);
    }
  }


        #ifdef ConsMemFromHeap
  ReleaseHeap(theMarkKey);
  LC_SetMemMgrDefault(context);
        #endif

  return(total_errors);
}


END_UGDIM_NAMESPACE
