// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
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

#include <new>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>

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
    throw std::bad_alloc();

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
      throw std::bad_alloc();

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
  Dune::dwarn << "increased coupling table, now " << n << " entries\n";

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

  assert(proc!=context.me());

#       if DebugCoupling<=1
  Dune::dvverb << "AddCoupling " << OBJ_GID(hdr)
               << " proc=" << proc << " prio=" << prio << "\n";
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
        cp2->prio = prio;
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
  int objIndex;

  assert(proc!=context.me());

#       if DebugCoupling<=1
  Dune::dvverb << "ModCoupling " << OBJ_GID(hdr)
               << " proc=" << proc << " prio=" << prio << "\n";
#       endif

  /* find or free position in coupling array */
  objIndex = OBJ_INDEX(hdr);
  if (! ObjHasCpl(context, hdr))
  {
    /* there are no couplings for this object! */
    Dune::dwarn << "ModCoupling: no couplings for " << OBJ_GID(hdr) << "\n";
    return(NULL);
  }
  else
  {
    /* look if coupling exists and change it */
    for (COUPLING *cp2 = IdxCplList(context, objIndex); cp2 != NULL; cp2 = CPL_NEXT(cp2))
    {
      if (CPL_PROC(cp2)==proc)
      {
        cp2->prio = prio;
        return(cp2);
      }
    }
  }

  /* coupling not found */
  DUNE_THROW(Dune::Exception,
             "no coupling from " << proc << " for " << OBJ_GID(hdr));
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
        Dune::dvverb << "DelCoupling " << OBJ_GID(hdr) << " on proc=" << proc
                     << ", now " << (IdxNCpl(context, objIndex)-1) << " cpls\n";
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


/*
 * DDD_InfoProcListRange
 */

DDD_InfoProcListRange::DDD_InfoProcListRange(DDDContext& context, const DDD_HDR hdr, bool includeDummy) noexcept
  : includeDummy_(includeDummy)
{
  dummy_._proc = context.me();
  dummy_.prio = OBJ_PRIO(hdr);

  const auto objIndex = OBJ_INDEX(hdr);
  if (objIndex < context.couplingContext().nCpls) {
    dummy_._next = IdxCplList(context, objIndex);
  }
  else {
    dummy_._next = nullptr;
  }
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
    return context.me();

  return context.procs();
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

  std::cout << "InfoCoupling for object " << OBJ_GID(hdr)
            << " (" << objIndex << "/" << nCpls << ")\n";

  if (objIndex < nCpls)
  {
    for(const COUPLING* cpl=IdxCplList(context, objIndex); cpl!=NULL; cpl=CPL_NEXT(cpl))
      std::cout << "    cpl " << cpl << " proc=" << CPL_PROC(cpl)
                << " prio=" << cpl->prio << "\n";
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
    throw std::bad_alloc();

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
