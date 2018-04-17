// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      cplmgr.c                                                      */
/*                                                                          */
/* Purpose:   management of couplings                                       */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/30 kb  begin                                            */
/*            94/08/24 kb  added DDD_InfoProcList()                         */
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

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include "dddi.h"

USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;

#define DebugCoupling 10  /* 10 is off */

START_UGDIM_NAMESPACE

using CplSegm = DDD::Mgr::CplSegm;

/*
        the storage of COUPLING items is done with the following scheme:
        allocation in segments of couplings, freeing into freelist.

        ALLOC:  try first to get one item out of freelist (memlistCpl),
                if that's not possible, get one from current segment;
                alloc segments from MemMgr.

        FREE:   put coupling into freelist.
 */

static CplSegm *NewCplSegm(DDD::DDDContext& context)
{
  auto& mctx = context.cplmgrContext();

  CplSegm *segm;

  segm = (CplSegm *) AllocTmpReq(sizeof(CplSegm), TMEM_CPL);

  if (segm==NULL)
  {
    DDD_PrintError('F', 2550, STR_NOMEM " during NewCoupling()");
    HARD_EXIT;
  }

  segm->next   = mctx.segmCpl;
  mctx.segmCpl = segm;
  segm->nItems = 0;
  mctx.nCplSegms++;

  return(segm);
}


static void FreeCplSegms(DDD::DDDContext& context)
{
  auto& mctx = context.cplmgrContext();

  CplSegm *segm = mctx.segmCpl;
  CplSegm *next = NULL;

  while (segm!=NULL)
  {
    next = segm->next;
    FreeTmpReq(segm, sizeof(CplSegm), TMEM_CPL);

    segm = next;
  }

  mctx.segmCpl = nullptr;
  mctx.nCplSegms = 0;
  mctx.memlistCpl = nullptr;
}


/****************************************************************************/

/* auxiliary function to init Coupling memory and initial data */
static void InitNewCoupling (COUPLING* cpl)
{
  /* set coupling to initial value, in order to find any bugs lateron.
     (hint by C.Wieners) */
  memset(cpl, 0, sizeof(COUPLING));

  /* init private data */
  cpl->_flags = 0;
}


static COUPLING *NewCoupling (DDD::DDDContext& context)
{
  auto& ctx = context.couplingContext();
  auto& mctx = context.cplmgrContext();

  COUPLING *cpl;

  if (DDD_GetOption(context, OPT_CPLMGR_USE_FREELIST)==OPT_ON)
  {
    /* allocate coupling from segments (which are allocated
       from segment-freelists) */

    if (mctx.memlistCpl == nullptr)
    {
      CplSegm *segm = mctx.segmCpl;

      if (segm==NULL || segm->nItems==CPLSEGM_SIZE)
      {
        segm = NewCplSegm(context);
      }

      cpl = &(segm->item[segm->nItems++]);
    }
    else
    {
      cpl = mctx.memlistCpl;
      mctx.memlistCpl = CPL_NEXT(cpl);
    }

    /* init coupling memory and its private data */
    InitNewCoupling(cpl);

    /* remember memory origin for later disposal */
    SETCPLMEM_FREELIST(cpl);
  }
  else
  {
    /* allocate coupling directly */
    cpl = (COUPLING *) AllocTmpReq(sizeof(COUPLING), TMEM_CPL);

    if (cpl==NULL)
    {
      DDD_PrintError('F', 2551, STR_NOMEM " during NewCoupling()");
      HARD_EXIT;
    }

    /* init coupling memory and its private data */
    InitNewCoupling(cpl);

    /* remember memory origin for later disposal */
    SETCPLMEM_EXTERNAL(cpl);
  }

  ctx.nCplItems += 1;

  return(cpl);
}


static void DisposeCoupling (DDD::DDDContext& context, COUPLING *cpl)
{
  auto& ctx = context.couplingContext();
  auto& mctx = context.cplmgrContext();

  if (CPLMEM(cpl)==CPLMEM_FREELIST)
  {
    CPL_NEXT(cpl) = mctx.memlistCpl;
    mctx.memlistCpl = cpl;
  }
  else
  {
    FreeTmpReq(cpl, sizeof(COUPLING), TMEM_CPL);
  }

  ctx.nCplItems -= 1;
}


/****************************************************************************/


static void AllocCplTables (DDD::DDDContext& context, long n)
{
  auto& ctx = context.couplingContext();

  /* allocate coupling table */
  ctx.cplTable.resize(n);
  ctx.nCplTable.resize(n);
}


static void IncreaseCplTabSize(DDD::DDDContext& context)
{
  auto& ctx = context.couplingContext();

  /* compute new size (currently: double size) */
  const std::size_t n = ctx.cplTable.size() * 2;

  /* allocate new coupling table */
  ctx.cplTable.resize(n);
  ctx.nCplTable.resize(n);

  /* issue a warning in order to inform user */
  sprintf(cBuffer, "increased coupling table, now %zd entries", n);
  DDD_PrintError('W', 2514, cBuffer);

  ddd_EnsureObjTabSize(context, n);
}




/****************************************************************************/
/*                                                                          */
/* Function:  AddCoupling                                                   */
/*                                                                          */
/* Purpose:   get new coupling record and init contents                     */
/*            if coupling is already there, no additional coupling is       */
/*            created. priority is adapted in this case.                    */
/*                                                                          */
/* Input:     hdr: DDD-header of object with new coupling                   */
/*            proc: owner of copy to be registered                          */
/*            prio: priority of copy                                        */
/*                                                                          */
/* Output:    ptr to new cpl record (or old one, if existed before)         */
/*            NULL on error                                                 */
/*                                                                          */
/****************************************************************************/

COUPLING *AddCoupling(DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc, DDD_PRIO prio)
{
  auto& ctx = context.couplingContext();

  COUPLING        *cp, *cp2;
  int objIndex;
  int freeCplIdx = ctx.nCpls;

  assert(proc!=me);

#       if DebugCoupling<=1
  sprintf(cBuffer, "%4d: AddCoupling %08x proc=%d prio=%d\n",
          me, OBJ_GID(hdr), proc, prio);
  DDD_PrintDebug(cBuffer);
#       endif

  /* find or free position in coupling array */
  objIndex = OBJ_INDEX(hdr);
  if (! ObjHasCpl(context, hdr))
  {
    if (freeCplIdx == ctx.cplTable.size())
    {
      /* try to make CplTables larger ... */
      IncreaseCplTabSize(context);
    }

    auto& objTable = context.objTable();
                #ifdef WithFullObjectTable
    DDD_HDR oldObj = objTable[freeCplIdx];

    /* exchange object without coupling and object with coupling */
    /* free position freeCplIdx, move corresponding hdr reference
       elsewhere. */
    objTable[objIndex] = oldObj;
    OBJ_INDEX(oldObj)      = objIndex;
                #else
    assert(IsHdrLocal(hdr));

    /* hdr has been local, therefore not known by DDD, we have
       to register it now. */
    context.nObjs(context.nObjs() + 1);
                #endif


    assert(freeCplIdx < context.objTable().size());
    objTable[freeCplIdx] = hdr;
    OBJ_INDEX(hdr)           = freeCplIdx;

    objIndex = freeCplIdx;
    IdxCplList(context, objIndex) = nullptr;
    IdxNCpl(context, objIndex) = 0;

    ctx.nCpls += 1;
  }
  else
  {
    for(cp2=IdxCplList(context, objIndex); cp2!=NULL; cp2=CPL_NEXT(cp2))
    {
      if (CPL_PROC(cp2)==proc)
      {
        if (cp2->prio!=prio)
        {
          /* coupling upgrades/downgrades, are they allowed?
                                                  printf("%4d: diff in cpl, %05x old %d-%d new %d-%d\n",
                                                          me,OBJ_GID(hdr),cp2->proc,cp2->prio, proc, prio);
           */
          cp2->prio = prio;
        }
        /*
                                        DDD_PrintError('W', 2600, "coupling already known in AddCoupling");
         */
        return(cp2);
      }
    }
  }

  /* create new coupling record */
  cp = NewCoupling(context);
  if (cp==NULL) {
    DDD_PrintError('E', 2500, STR_NOMEM " in AddCoupling");
    return(NULL);
  }

  /* init contents */
  cp->obj = hdr;
  CPL_PROC(cp) = proc;
  cp->prio = prio;

  /* insert into theCpl array */
  CPL_NEXT(cp) = IdxCplList(context, objIndex);
  IdxCplList(context, objIndex) = cp;
  IdxNCpl(context, objIndex)++;

  return(cp);
}





/****************************************************************************/
/*                                                                          */
/* Function:  ModCoupling                                                   */
/*                                                                          */
/* Purpose:   find existing coupling record and modify priority             */
/*            this function does coupling upgrade/downgrade without         */
/*            complaining.                                                  */
/*                                                                          */
/* Input:     hdr: DDD-header of object with new coupling                   */
/*            proc: owner of copy to be modified                            */
/*            prio: new priority of copy                                    */
/*                                                                          */
/* Output:    ptr to old cpl record                                         */
/*            NULL on error                                                 */
/*                                                                          */
/****************************************************************************/

COUPLING *ModCoupling(DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc, DDD_PRIO prio)
{
  COUPLING        *cp2;
  int objIndex;

  assert(proc!=me);

#       if DebugCoupling<=1
  sprintf(cBuffer, "%4d: ModCoupling %08x proc=%d prio=%d\n",
          me, OBJ_GID(hdr), proc, prio);
  DDD_PrintDebug(cBuffer);
#       endif

  /* find or free position in coupling array */
  objIndex = OBJ_INDEX(hdr);
  if (! ObjHasCpl(context, hdr))
  {
    /* there are no couplings for this object! */
    sprintf(cBuffer, "no couplings for " OBJ_GID_FMT " in ModCoupling", OBJ_GID(hdr));
    DDD_PrintError('E', 2530, cBuffer);
    return(NULL);
  }
  else
  {
    /* look if coupling exists and change it */
    for(cp2=IdxCplList(context, objIndex); cp2!=NULL; cp2=CPL_NEXT(cp2))
    {
      if (CPL_PROC(cp2)==proc)
      {
        cp2->prio = prio;
        return(cp2);
      }
    }
  }

  /* coupling not found */
  sprintf(cBuffer, "no coupling from %d for " OBJ_GID_FMT " in ModCoupling",
          proc, OBJ_GID(hdr));
  DDD_PrintError('E', 2531, cBuffer);
  HARD_EXIT;

  return(NULL);         /* never reach this */
}




/****************************************************************************/
/*                                                                          */
/* Function:  DelCoupling                                                   */
/*                                                                          */
/* Purpose:   remove coupling record from object                            */
/*                                                                          */
/* Input:     hdr: DDD-header of object with old coupling                   */
/*            proc: owner of copy to be un-registered                       */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DelCoupling (DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc)
{
  COUPLING        *cpl, *cplLast;
  auto& objTable = context.objTable();
  const int objIndex = OBJ_INDEX(hdr);
  auto& ctx = context.couplingContext();

  if (objIndex < ctx.nCpls)
  {
    for(cpl=IdxCplList(context, objIndex), cplLast=NULL; cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      if(CPL_PROC(cpl)==proc)
      {
        if (cplLast==NULL)
        {
          IdxCplList(context, objIndex) = CPL_NEXT(cpl);
        }
        else {
          CPL_NEXT(cplLast) = CPL_NEXT(cpl);
        }
#                               if DebugCoupling<=1
        sprintf(cBuffer,"%4d: DelCoupling %07x on proc=%d, now %d cpls\n",
                me, OBJ_GID(hdr), proc, IdxNCpl(context, objIndex)-1);
        DDD_PrintDebug(cBuffer);
#                               endif

        DisposeCoupling(context, cpl);

        IdxNCpl(context, objIndex)--;

        if (IdxNCpl(context, objIndex)==0)
        {
          ctx.nCpls -= 1;

                                        #ifdef WithFullObjectTable
          OBJ_INDEX(hdr) = ctx.nCpls;
          OBJ_INDEX(objTable[ctx.nCpls]) = objIndex;
          objTable[objIndex] = objTable[ctx.nCpls];
          objTable[ctx.nCpls] = hdr;
                                        #else
          /* we will not register objects without coupling,
             so we have to forget about hdr and mark it as local. */
          context.nObjs(context.nObjs() - 1);
          assert(context.nObjs() == ctx.nCpls);

          objTable[objIndex] = objTable[ctx.nCpls];
          OBJ_INDEX(objTable[ctx.nCpls]) = objIndex;

          MarkHdrLocal(hdr);
                                        #endif

          IdxCplList(context, objIndex) = IdxCplList(context, ctx.nCpls);
          IdxNCpl(context, objIndex) = IdxNCpl(context, ctx.nCpls);
        }
        break;
      }
      cplLast = cpl;
    }
  }
}


/****************************************************************************/
/*                                                                          */
/* Function:  DisposeCouplingList                                           */
/*                                                                          */
/* Purpose:   dispose complete coupling list                                */
/*                                                                          */
/* Input:     cpl: first element of coupling list                           */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DisposeCouplingList (DDD::DDDContext& context, COUPLING *cpl)
{
  COUPLING *c, *next;

  c = cpl;
  while (c!=NULL)
  {
    next = CPL_NEXT(c);
    DisposeCoupling(context, c);
    c = next;
  }
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_InfoProcList                                              */
/*                                                                          */
/* Purpose:   return list of couplings of certain object                    */
/*                                                                          */
/* Input:     hdr: DDD-header of object with coupling                       */
/*                                                                          */
/* Output:    pointer to localIBuffer, which has been filled with:          */
/*               1) id of calling processor                                 */
/*               2) priority of local object copy on calling processor      */
/*               3) id of processor which holds an object copy              */
/*               4) priority of copy on that processor                      */
/*               5) 3+4 repeated for each coupling                          */
/*               6) processor number = -1 as end mark                       */
/*                                                                          */
/****************************************************************************/

int *DDD_InfoProcList (DDD::DDDContext& context, DDD_HDR hdr)
{
  auto& mctx = context.cplmgrContext();

COUPLING *cpl;
int i, objIndex = OBJ_INDEX(hdr);

/* insert description of own (i.e. local) copy */
mctx.localIBuffer[0] = me;
mctx.localIBuffer[1] = OBJ_PRIO(hdr);

i=2;

/* append descriptions of foreign copies */
if (objIndex < context.couplingContext().nCpls)
{
  for(cpl=IdxCplList(context, objIndex); cpl!=NULL; cpl=CPL_NEXT(cpl), i+=2) {
    mctx.localIBuffer[i]   = CPL_PROC(cpl);
    mctx.localIBuffer[i+1] = cpl->prio;
  }
}

/* append end mark */
mctx.localIBuffer[i] = -1;

return(mctx.localIBuffer);
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_InfoProcPrio                                              */
/*                                                                          */
/* Purpose:   return first processor number with a given priority           */
/*                                                                          */
/* Input:     hdr:  DDD-header of object with coupling                      */
/*            prio: priority to search for                                  */
/*                                                                          */
/* Output:    id of processor which holds the object copy with prio         */
/*            (or procs if no such copy exists)                             */
/*                                                                          */
/****************************************************************************/

DDD_PROC DDD_InfoProcPrio(const DDD::DDDContext& context, DDD_HDR hdr, DDD_PRIO prio)
{
  COUPLING *cpl;
  int objIndex = OBJ_INDEX(hdr);

  /* append descriptions of foreign copies */
  if (objIndex < context.couplingContext().nCpls)
  {
    for(cpl=IdxCplList(context, objIndex); cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      if (cpl->prio == prio)
        return(CPL_PROC(cpl));
    }
  }

  /* eventually local copy has priority we are looking for */
  if (OBJ_PRIO(hdr)==prio)
    return(me);

  return(procs);
}


bool DDD_InfoIsLocal(const DDD::DDDContext& context, DDD_HDR hdr)
{
  return(! ObjHasCpl(context, hdr));
}


int DDD_InfoNCopies(const DDD::DDDContext& context, DDD_HDR hdr)
{
  /*
     COUPLING *cpl;
     int n = 0;

     if (ObjHasCpl(context, hdr))
     {
          for(cpl=IdxCplList(context, OBJ_INDEX(hdr)); cpl!=NULL; cpl=CPL_NEXT(cpl))
                  n++;
     }
   */

  return(ObjNCpl(context, hdr));
}


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_InfoCoupling                                              */
/*                                                                          */
/* Purpose:   displays list of coupling for certain object                  */
/*                                                                          */
/* Input:     hdr: DDD-header of object with coupling                       */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void DDD_InfoCoupling(const DDD::DDDContext& context, DDD_HDR hdr)
{
  int objIndex = OBJ_INDEX(hdr);
  const auto& nCpls = context.couplingContext().nCpls;

  sprintf(cBuffer, "%4d: InfoCoupling for object " OBJ_GID_FMT " (%05d/%05d)\n",
          me, OBJ_GID(hdr), objIndex, nCpls);
  DDD_PrintLine(cBuffer);

  if (objIndex < nCpls)
  {
    for(const COUPLING* cpl=IdxCplList(context, objIndex); cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      sprintf(cBuffer, "%4d:    cpl %08x proc=%4d prio=%4d\n",
              me, cpl, CPL_PROC(cpl), cpl->prio);
      DDD_PrintLine(cBuffer);
    }
  }
}




/****************************************************************************/
/*                                                                          */
/* Function:  DDD_InfoCplMemory                                             */
/*                                                                          */
/* Purpose:   returns number of bytes used for coupling data                */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    size of memory used for couplings                             */
/*                                                                          */
/****************************************************************************/

size_t DDD_InfoCplMemory(const DDD::DDDContext& context)
{
  return sizeof(CplSegm) * context.cplmgrContext().nCplSegms;
}



/****************************************************************************/
/*                                                                          */
/* Function:  CplMgrInit and CplMgrExit                                     */
/*                                                                          */
/* Purpose:   init/exit coupling manager                                    */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

void ddd_CplMgrInit(DDD::DDDContext& context)
{
  auto& mctx = context.cplmgrContext();

  /* allocate first (smallest) coupling tables */
  AllocCplTables(context, MAX_CPL_START);


  mctx.localIBuffer = (int*)AllocFix((2*context.procs()+1)*sizeof(int));
  if (mctx.localIBuffer == nullptr)
  {
    DDD_PrintError('E', 2532, STR_NOMEM " for DDD_InfoProcList()");
    HARD_EXIT;
  }

  mctx.memlistCpl = nullptr;
  mctx.segmCpl    = nullptr;
  mctx.nCplSegms  = 0;
}


void ddd_CplMgrExit(DDD::DDDContext& context)
{
  auto& ctx = context.couplingContext();
  auto& mctx = context.cplmgrContext();

  FreeFix(mctx.localIBuffer);
  FreeCplSegms(context);

  ctx.cplTable.clear();
  ctx.nCplTable.clear();
}

/****************************************************************************/

END_UGDIM_NAMESPACE
