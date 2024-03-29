// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      xfer.c                                                        */
/*                                                                          */
/* Purpose:   main module for object transfer                               */
/*            (contains basic functionality used by the rest of the source  */
/*             files in the Xfer-module)                                    */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/30 kb  begin                                            */
/*            95/03/21 kb  added variable sized objects (XferCopyObjX)      */
/*            96/07/03 kb  split XferInfo-list into ObjXfer and CplXfer  */
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

#include <algorithm>
#include <iomanip>
#include <tuple>

#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddi.h>
#include "xfer.h"

using namespace PPIF;

  START_UGDIM_NAMESPACE


/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/



static bool sort_NewOwners (const XICopyObj* a, const XICopyObj* b)
{
  return std::tie(a->gid, a->dest) < std::tie(b->gid, b->dest);
}


/****************************************************************************/

/*
        collect a temporary list of XINewCpl-items.

        for each XICopyObj-command whose destination doesn't have
        an object copy already do: create a set of XINewCpl-items, one
        for every processor which owns a copy of the local object.

        this is an estimate, because without communication the sending
        processor cannot know whether the object copy will be accepted.
        this final information will be transferred in a second pass, as
        soon as the receiver has decided whether he accepts the incoming
        object or not (depending on rules XFER-C2, XFER-C3, XFER-C4,
        XFER-P and XFER-D).
 */

XICopyObj **CplClosureEstimate (DDD::DDDContext& context, const std::vector<XICopyObj*>& arrayItems, int *nRet)
{
  const auto& me = context.me();

  int i, nNewOwners;
  XICopyObj **arrayNewOwners = NULL;
  XICopyObj* const* items = arrayItems.data();
  const int n = arrayItems.size();



  nNewOwners=0;
  for(i=0; i<n; i++)
  {
    XICopyObj *xi = items[i];
    DDD_PROC dest = xi->dest;              /* destination proc */
    COUPLING *cpl, *xicpl = ObjCplList(context, xi->hdr);
    DDD_GID xigid = xi->gid;
    DDD_TYPE xitype = OBJ_TYPE(xi->hdr);

    SET_CO_NEWOWNER(xi);

    /* look if there's a coupling for dest */
    for(cpl=xicpl; cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      if (dest==CPL_PROC(cpl))
      {
        /* got one coupling, destination is not a new owner */
        CLEAR_CO_NEWOWNER(xi);

        /* destination proc had a copy before xfer */
        /* check whether priority of that copy will change */
        /*
                                        if (WhichPrioWins(xi->prio, cpl->prio)==1)
                                        {
         */
        /* new prio will win on other proc -> adapt coupling */
        /*
           printf("%4d: XXXCoupling %08x proc=%d prio=%d\n",
           me,xigid,dest,xi->prio);
                                                cpl->prio = xi->prio;
                                        }
         */

        /* this should be the only coupling for that proc.
           leave loop prematurely. */
        break;
      }
    }


    if (CO_NEWOWNER(xi))
    {
      nNewOwners++;

      /* destination proc didn't have a copy before xfer */

      /* inform other owners of local copies (XINewCpl) */
      for(cpl=xicpl; cpl!=NULL; cpl=CPL_NEXT(cpl))
      {
        XINewCpl *xc = NewXINewCpl(context);
        if (xc==NULL)
          throw std::bad_alloc();

        xc->to      = CPL_PROC(cpl);                         /* receiver of XINewCpl    */
        NewCpl_SetDest(xc->te,dest);                         /* destination of XICopyObj*/
        NewCpl_SetGid(xc->te,xigid);                         /* the object's gid        */
        NewCpl_SetPrio(xc->te,xi->prio);                         /* new obj's priority  */
        NewCpl_SetType(xc->te,xitype);                           /* the object's type   */
      }

      /* send current couplings (XIOldCpl) to new destination */
      /* note: destination proc can get this information
               multiple times, once for each incoming object
               with same gid (from different senders)  */
      for(cpl=xicpl; cpl!=NULL; cpl=CPL_NEXT(cpl))
      {
        XIOldCpl *xc = NewXIOldCpl(context);
        if (xc==NULL)
          throw std::bad_alloc();

        xc->to      = dest;                                    /* receiver of XIOldCpl */
        xc->te.gid  = xigid;                                   /* the object's gid     */
        xc->te.proc = CPL_PROC(cpl);                           /* coupling proc        */
        xc->te.prio = cpl->prio;                               /* coupling priority    */
      }

      /* send one coupling (XIOldCpl) for local copy */
      {
        XIOldCpl *xc = NewXIOldCpl(context);
        if (xc==NULL)
          throw std::bad_alloc();

        xc->to      = dest;                                     /* receiver of XIOldCpl */
        xc->te.gid  = xigid;                                    /* the object's gid     */
        xc->te.proc = me;                                       /* coupling proc        */
        xc->te.prio = OBJ_PRIO(xi->hdr);                        /* coupling priority    */
      }
    }
  }

  *nRet = nNewOwners;
  /*
     printf("%4d: nNewOwners=%d\n", me, nNewOwners);
   */

  /* check multiple new-owner-destinations for same gid */
  if (nNewOwners>0)
  {
    int j, k;

    arrayNewOwners =
      (XICopyObj **) OO_Allocate (sizeof(XICopyObj *)* nNewOwners);
    if (arrayNewOwners==NULL)
    {
      DDD_PrintError('E', 6102, STR_NOMEM " in XferEnd()");
      return NULL;
    }

    /* fill pointer array XICopyObj-items marked CO_NEWOWNER */
    for(j=0, k=0; j<n; j++)
    {
      if (CO_NEWOWNER(items[j]))
      {
        arrayNewOwners[k] = items[j];
        k++;
      }
    }

    if (nNewOwners==1)
      return(arrayNewOwners);

    /* sort according to gid (items is sorted according to dest) */
    std::sort(arrayNewOwners, arrayNewOwners + nNewOwners, sort_NewOwners);


    for(j=0; j<nNewOwners-1; j++)
    {
      XICopyObj *no1 = arrayNewOwners[j];
      DDD_GID gid1 = no1->gid;

      for(k=j+1; k<nNewOwners; k++)
      {
        XICopyObj *no2 = arrayNewOwners[k];
        DDD_TYPE no2type;

        if (no2->gid != gid1)
          break;

        no2type = OBJ_TYPE(no2->hdr);

        /* inform other new-owners of same obj (also XINewCpl!)    */
        /* tell no1-dest that no2-dest gets a copy with no2->prio  */
        {
          XINewCpl *xc = NewXINewCpl(context);
          if (xc==NULL)
            throw std::bad_alloc();

          xc->to      = no1->dest;                                 /* receiver of XINewCpl     */
          NewCpl_SetDest(xc->te,no2->dest);                               /* dest of XICopyObj */
          NewCpl_SetGid(xc->te,gid1);                                     /* the obj's gid     */
          NewCpl_SetPrio(xc->te,no2->prio);                               /* new obj's priority*/
          NewCpl_SetType(xc->te,no2type);                                 /* the obj's type    */
        }
        /* tell no2->dest that no1-dest gets a copy with no1->prio */
        {
          XINewCpl *xc = NewXINewCpl(context);
          if (xc==NULL)
            throw std::bad_alloc();

          xc->to      = no2->dest;                                 /* receiver of XINewCpl     */
          NewCpl_SetDest(xc->te,no1->dest);                               /* dest of XICopyObj */
          NewCpl_SetGid(xc->te,gid1);                                     /* the obj's gid     */
          NewCpl_SetPrio(xc->te,no1->prio);                               /* new obj's priority*/
          NewCpl_SetType(xc->te,no2type);                                 /* the obj's type    */
        }
      }
    }
  }

  return(arrayNewOwners);
}


/****************************************************************************/

/*
        auxiliary functions for PrepareObjMsgs()
 */


static void BuildDepDataInfo (XFERMSG *xm, XICopyObj *xi)
{
  XFERADDDATA *xa;
  int ptr, chunks;


  /* count characteristic values for each chunk */
  chunks = ptr = 0;
  for(xa=xi->add; xa!=NULL; xa=xa->next)
  {
    ptr += xa->addNPointers;

    /* add control information size for var-sized AddData-Items */
    if (xa->sizes!=NULL)
      xi->addLen += CEIL(sizeof(int) * xa->addCnt);

    chunks++;
  }


  /* add size of control information */
  if (xi->addLen>0)
    xi->addLen += CEIL(sizeof(int)) + chunks*CEIL(2*sizeof(int));

  /* add to current message size information */
  xm->size      += xi->addLen;
  xm->nPointers += ptr;
}


static XFERMSG *CreateXferMsg (DDD_PROC dest, XFERMSG *lastxm)
{
  XFERMSG *xm;

  xm = (XFERMSG *) OO_Allocate (sizeof(XFERMSG));
  if (xm==NULL)
  {
    DDD_PrintError('E', 6100, STR_NOMEM " in PrepareObjMsgs");
    return NULL;
  }
  xm->nPointers  = 0;
  xm->nObjects   = 0;
  xm->proc = dest;
  xm->size = 0;

  xm->xferObjArray = NULL;
  xm->xferNewCpl   = NULL;
  xm->xferOldCpl   = NULL;
  xm->nObjItems = 0;
  xm->nNewCpl   = 0;
  xm->nOldCpl   = 0;

  xm->next = lastxm;

  return xm;
}



static XFERMSG *AccumXICopyObj (const DDD::DDDContext& context,
                                XFERMSG *currxm, int *nMsgs, int *nItems,
                                XICopyObj **items, DDD_PROC dest, int nmax)
{
  XFERMSG *xm;
  int i;

  if (currxm!=NULL && currxm->proc==dest)
  {
    /* there is a XFERMSG with correct processor number -> reuse it */
    xm = currxm;
  }
  else
  {
    /* create new XFERMSG structure */
    xm = CreateXferMsg(dest, currxm);
    (*nMsgs)++;
  }

#       if DebugXfer<=2
  Dune::dvverb << "PrepareObjMsgs, XferMsg"
               << " proc=" << dest << " nmax=" << nmax << "\n";
#       endif


  for (i=0; i<nmax && items[i]->dest==dest; i++)
  {
    XICopyObj *xi = items[i];
    DDD_HDR hdr = xi->hdr;
    const TYPE_DESC& desc = context.typeDefs()[OBJ_TYPE(hdr)];

#               if DebugXfer<=0
    Dune::dvverb
      << "PrepareObjMsgs, proc=" << dest
      << " i=" i << "/" << nmax << " (" << xi->gid << ")\n";
#               endif

    /* accumulate xfer-items in message-info */
    xm->nObjects++;

    /* length of object itself, possibly variable  */
    xm->size += CEIL(xi->size);
    xm->nPointers += desc.nPointers;

    if (xi->add != NULL)
      BuildDepDataInfo(xm, xi);
  }

  *nItems = i;
  return xm;
}




static XFERMSG *AccumXINewCpl (XFERMSG *currxm, int *nMsgs, int *nItems,
                               XINewCpl **items, DDD_PROC dest, int nmax)
{
  XFERMSG *xm;
  int i;

  if (currxm!=NULL && currxm->proc==dest)
  {
    /* there is a XFERMSG with correct processor number -> reuse it */
    xm = currxm;
  }
  else
  {
    /* create new XFERMSG structure */
    xm = CreateXferMsg(dest, currxm);
    (*nMsgs)++;
  }

#       if DebugXfer<=2
  Dune::dvverb << "PrepareObjMsgs, XferMsg"
               << " proc=" << dest << " nmax=" << nmax << "\n";
#       endif


  for (i=0; i<nmax && items[i]->to==dest; i++)
  {
#               if DebugXfer<=0
    XINewCpl *xi = items[i];
    Dune::dvverb
      << "PrepareObjMsgs, proc=" << dest
      << " i=" i << "/" << nmax << " (" << NewCpl_GetGid(xi->te) << ")\n";
#               endif
  }

  *nItems = i;
  return xm;
}



static XFERMSG *AccumXIOldCpl (XFERMSG *currxm, int *nMsgs, int *nItems,
                               XIOldCpl **items, DDD_PROC dest, int nmax)
{
  XFERMSG *xm;
  int i;

  if (currxm!=NULL && currxm->proc==dest)
  {
    /* there is a XFERMSG with correct processor number -> reuse it */
    xm = currxm;
  }
  else
  {
    /* create new XFERMSG structure */
    xm = CreateXferMsg(dest, currxm);
    (*nMsgs)++;
  }

#       if DebugXfer<=2
  Dune::dvverb << "PrepareObjMsgs, XferMsg"
               << " proc=" << dest << " nmax=" << nmax << "\n";
#       endif


  for (i=0; i<nmax && items[i]->to==dest; i++)
  {
#               if DebugXfer<=0
    XIOldCpl *xi = items[i];
    Dune::dvverb
      << "PrepareObjMsgs, proc=" << dest
      << " i=" i << "/" << nmax << " (" << xi->te.gid << ")\n";
#               endif
  }

  *nItems = i;
  return xm;
}



/****************************************************************************/

/*
        prepare messages for phase 1.

        object copies will be sent as well as the estimated
        coupling closure from CplClosureEstimate().
 */

int PrepareObjMsgs (DDD::DDDContext& context,
                    std::vector<XICopyObj*>& arrayO,
                    XINewCpl **itemsNC, int nNC,
                    XIOldCpl **itemsOC, int nOC,
                    XFERMSG **theMsgs, size_t *memUsage)
{
  auto& ctx = context.xferContext();
  const auto& procs = context.procs();

  XFERMSG    *xm=NULL;
  int iO, iNC, iOC, nMsgs=0;

  XICopyObj** itemsO = arrayO.data();
  const int nO = arrayO.size();


#       if DebugXfer<=3
  Dune::dverb << "PrepareObjMsgs,"
              << " nXICopyObj=" << nO
              << " nXINewCpl=" << nNC
              << " nXIOldCpl=" << nOC << "\n";
#       endif


  /*
          run through both itemsO and itemsNC/itemsOC simultaneously,
          each time a new proc-nr is encountered in one of these
          lists, create a new XFERMSG item.

          (the lists have been sorted according to proc-nr previously.)
   */

  iO=0; iNC=0; iOC=0;
  while (iO<nO || iNC<nNC || iOC<nOC)
  {
    int n;
    DDD_PROC pO = (iO<nO) ? itemsO[iO]->dest : procs;
    DDD_PROC pNC = (iNC<nNC) ? itemsNC[iNC]->to   : procs;
    DDD_PROC pOC = (iOC<nOC) ? itemsOC[iOC]->to   : procs;

    if (pO<=pNC && pO<=pOC && pO<procs)
    {
      xm = AccumXICopyObj(context, xm, &nMsgs, &n, itemsO+iO, pO, nO-iO);
      xm->xferObjArray = itemsO+iO;
      xm->nObjItems = n;
      iO += n;
    }

    if (pNC<=pO && pNC<=pOC && pNC<procs)
    {
      xm = AccumXINewCpl(xm, &nMsgs, &n, itemsNC+iNC, pNC, nNC-iNC);
      xm->xferNewCpl = itemsNC+iNC;
      xm->nNewCpl = n;
      iNC += n;
    }

    if (pOC<=pO && pOC<=pNC && pOC<procs)
    {
      xm = AccumXIOldCpl(xm, &nMsgs, &n, itemsOC+iOC, pOC, nOC-iOC);
      xm->xferOldCpl = itemsOC+iOC;
      xm->nOldCpl = n;
      iOC += n;
    }

    if (pO==procs) iO = nO;
    if (pNC==procs) iNC = nNC;
    if (pOC==procs) iOC = nOC;
  }
  *theMsgs = xm;


  /* compute brutto message size from netto message size */
  for(xm=*theMsgs; xm!=NULL; xm=xm->next)
  {
    size_t bufSize;
    xm->msg_h = LC_NewSendMsg(context, ctx.objmsg_t, xm->proc);
    LC_SetTableSize(xm->msg_h, ctx.symtab_id, xm->nPointers);
    LC_SetTableSize(xm->msg_h, ctx.objtab_id, xm->nObjects);
    LC_SetTableSize(xm->msg_h, ctx.newcpl_id, xm->nNewCpl);
    LC_SetTableSize(xm->msg_h, ctx.oldcpl_id, xm->nOldCpl);
    LC_SetChunkSize(xm->msg_h, ctx.objmem_id, xm->size);

    bufSize = LC_MsgFreeze(xm->msg_h);
    *memUsage += bufSize;

    if (DDD_GetOption(context, OPT_INFO_XFER) & XFER_SHOW_MEMUSAGE)
    {
      using std::setw;
      Dune::dwarn
        << "DDD MESG [" << setw(3) << "]: SHOW_MEM send msg "
        << " dest=" << setw(4) << xm->proc
        << " size=" << setw(10) << bufSize << "\n";
    }
  }


#       if DebugXfer<=3
  Dune::dverb << "PrepareObjMsgs, nMsgs=" << nMsgs << "\n";
#       endif

  return(nMsgs);
}



/****************************************************************************/

/*
        execute SetPrio-commands and create those XIModCpl-items,
        which can be computed without knowledge of information sent by other
        procs during first message phase.
 */
void ExecLocalXISetPrio (
  DDD::DDDContext& context,
  const std::vector<XISetPrio*>& arrayP,
  XIDelObj  **itemsD, int nD,
  XICopyObj  **itemsNO, int nNO)
{
  int iP, iD, iNO;
  XISetPrio* const* itemsP = arrayP.data();
  const int nP = arrayP.size();

  /*
          execute SetPrio only if no corresponding DelObj exists!
   */
  for(iP=0, iD=0, iNO=0; iP<nP; iP++)
  {
    XISetPrio *sp = itemsP[iP];
    DDD_HDR hdr = sp->hdr;
    DDD_GID gid      = sp->gid;
    DDD_PRIO newprio  = sp->prio;

    while ((iD<nD) && (itemsD[iD]->gid<gid))
      iD++;

    /* skip XICopyObj-items until entries for gid found */
    while (iNO<nNO && itemsNO[iNO]->gid<gid)
      iNO++;


    sp->is_valid = (! ((iD<nD) && (itemsD[iD]->gid==gid)));

    if (sp->is_valid)
    {
      /* SetPrio, but _no_ DelObj: execute SetPrio */
      DDD_TYPE typ   = OBJ_TYPE(hdr);
      const TYPE_DESC& desc = context.typeDefs()[typ];

      /* call application handler for changing prio of dependent objects */
      if (desc.handlerSETPRIORITY)
      {
        DDD_OBJ obj = HDR2OBJ(hdr, &desc);

        desc.handlerSETPRIORITY(context, obj, newprio);
      }

      /* change actual priority to new value */
      OBJ_PRIO(hdr) = newprio;


      /* generate XIModCpl-items */

      /* 1. for all existing couplings */
      for (COUPLING *cpl = ObjCplList(context, hdr); cpl != NULL; cpl = CPL_NEXT(cpl))
      {
        XIModCpl *xc = NewXIModCpl(context);
        if (xc==NULL)
          throw std::bad_alloc();

        xc->to      = CPL_PROC(cpl);                           /* receiver of XIModCpl  */
        xc->te.gid  = gid;                                     /* the object's gid      */
        xc->te.prio = newprio;                                 /* the object's new prio */
        xc->typ     = typ;                                     /* the object's type     */
      }
      /* 2. for all CopyObj-items with new-owner destinations */
      while (iNO<nNO && itemsNO[iNO]->gid==gid)
      {
        XIModCpl *xc = NewXIModCpl(context);
        if (xc==NULL)
          throw std::bad_alloc();

        xc->to      = itemsNO[iNO]->dest;                        /* receiver of XIModCpl */
        xc->te.gid  = gid;                                       /* the object's gid     */
        xc->te.prio = newprio;                                  /* the object's new prio */
        xc->typ     = typ;                                     /* the object's type     */

        iNO++;
      }
    }
    /*
            else: SetPrio _and_ DelObj, SetPrio is invalid,
                  DelObj will be executed lateron
                  (this is rule XFER-M1).
     */
  }

}



/*
        execute local DelObj-commands and create those XIDelCpl-items,
        which can be computed without knowledge of information sent by other
        procs during first message phase.
 */


void ExecLocalXIDelCmd (DDD::DDDContext& context, XIDelCmd  **itemsD, int nD)
{
  int iD;
  XIDelCmd **origD;

  if (nD==0)
    return;

  /* reconstruct original order of DelObj commands */
  origD = (XIDelCmd **) OO_Allocate (sizeof(XIDelCmd *) * nD);
  if (origD==NULL)
    throw std::bad_alloc();

  /* copy pointer array and resort it */
  memcpy(origD, itemsD, sizeof(XIDelCmd *) * nD);
  OrigOrderXIDelCmd(context, origD, nD);


  /* loop in original order (order of Del-cmd issuing) */
  for(iD=0; iD<nD; iD++)
  {
    DDD_HDR hdr = origD[iD]->hdr;
    DDD_TYPE typ   = OBJ_TYPE(hdr);
    const TYPE_DESC& desc = context.typeDefs()[typ];
    DDD_OBJ obj   = HDR2OBJ(hdr, &desc);

    /* do deletion */
    if (desc.handlerDELETE)
      desc.handlerDELETE(context, obj);
    else
    {
      /* TODO the following three calls should be collected in
         one ObjMgr function */

      /* destruct LDATA and GDATA */
      if (desc.handlerDESTRUCTOR)
        desc.handlerDESTRUCTOR(context, obj);

      /* HdrDestructor will call ddd_XferRegisterDelete() */
      DDD_HdrDestructor(context, hdr);
      DDD_ObjDelete(obj, desc.size, typ);
    }
  }

  OO_Free (origD /*,0*/);
}




void ExecLocalXIDelObj (
  DDD::DDDContext& context,
  XIDelObj  **itemsD, int nD,
  XICopyObj  **itemsNO, int nNO)
{
  int iD, iNO;


  /* create XIDelCpl for all DelObj-commands (sorted acc. to gid) */
  for(iD=0, iNO=0; iD<nD; iD++)
  {
    DDD_GID gid   = itemsD[iD]->gid;


    /* skip XICopyObj-items until entries for gid found */
    while (iNO<nNO && itemsNO[iNO]->gid<gid)
      iNO++;


    /* generate XIDelCpl-items */
    /* 1. for all existing couplings, has been done during
          ddd_XferRegisterDelete. */

    /* 2. for all CopyObj-items with new-owner destinations */
    while (iNO<nNO && itemsNO[iNO]->gid==gid)
    {
      XIDelCpl *xc = NewXIDelCpl(context);
      if (xc==NULL)
        throw std::bad_alloc();

      xc->to      = itemsNO[iNO]->dest;                  /* receiver of XIDelCpl */
      xc->prio    = PRIO_INVALID;                        /* dont remember priority   */
      xc->te.gid  = gid;                                 /* the object's gid     */


      /* we must remember couplings for eventual restoring
         (if this object is received from another proc) */
      xc->next = itemsD[iD]->delcpls;
      itemsD[iD]->delcpls = xc;

      iNO++;
    }
  }
}




/****************************************************************************/


/*
        create those XI???Cpl-items, which require knowledge of information
        sent by other procs during first message phase.
 */
void PropagateCplInfos (
  DDD::DDDContext& context,
  XISetPrio **itemsP, int nP,
  XIDelObj  **itemsD, int nD,
  TENewCpl  *arrayNC, int nNC)
{
  int iP, iD, iNC;

  /*
          step 1: create XIModCpl-items from SetPrio-cmds
                  (only if no DelObj-items exist)
   */
  for(iP=0, iNC=0; iP<nP; iP++)
  {
    XISetPrio *sp = itemsP[iP];

    if (sp->is_valid)
    {
      DDD_HDR hdr = sp->hdr;
      DDD_GID gid      = sp->gid;
      DDD_PRIO newprio  = sp->prio;

      /* skip TENewCpl-entries until one for gid found */
      while (iNC<nNC && NewCpl_GetGid(arrayNC[iNC])<gid)
        iNC++;

      /* generate additional XIModCpl-items for all valid NewCpl-items */
      while (iNC<nNC && NewCpl_GetGid(arrayNC[iNC])==gid)
      {
        XIModCpl *xc = NewXIModCpl(context);
        if (xc==NULL)
          throw std::bad_alloc();

        /* receiver of XIModCpl */
        xc->to      = NewCpl_GetDest(arrayNC[iNC]);
        xc->te.gid  = gid;                                       /* the object's gid     */
        xc->te.prio = newprio;                                   /* the object's new prio */
        xc->typ     = OBJ_TYPE(hdr);                             /* the object's type     */

        iNC++;
      }
    }
  }



  /*
          step 2: create XIDelCpl-items from DelObj-cmds
   */
  for(iD=0, iNC=0; iD<nD; iD++)
  {
    DDD_GID gid   = itemsD[iD]->gid;

    /* skip TENewCpl-entries until one for gid found */
    while (iNC<nNC && NewCpl_GetGid(arrayNC[iNC])<gid)
      iNC++;

    /* generate additional XIDelCpl-items for all valid NewCpl-items */
    while (iNC<nNC && NewCpl_GetGid(arrayNC[iNC])==gid)
    {
      XIDelCpl *xc = NewXIDelCpl(context);
      if (xc==NULL)
        throw std::bad_alloc();

      xc->to      = NewCpl_GetDest(arrayNC[iNC]);                   /* receiver of XIDelCpl */
      xc->prio    = PRIO_INVALID;
      xc->te.gid  = gid;                                 /* the object's gid     */
      /*
         printf("%4d: DelCpl 3      %08x %d %d\n",me,gid,xc->to,xc->prio);
       */

      iNC++;
    }
  }
}





/****************************************************************************/


/*
        this function is called by DDD_HdrDestructor!
 */
void ddd_XferRegisterDelete (DDD::DDDContext& context, DDD_HDR hdr)
{
  COUPLING *cpl;
  XIDelObj *xi;

  /* create new XIDelObj */
  xi      = NewXIDelObj(context);
  if (xi==NULL)
    throw std::bad_alloc();

  xi->gid = OBJ_GID(hdr);
  xi->delcpls = NULL;

  /*
          now generate XIDelCpl-items, one for each existing coupling.
          these items serve as notification of this delete operation
          for remote processors with same object.
          these items are also a intermediate storage for the object's
          coupling list, in case the object is received after deletion
          and the coupling list must be restored.
   */
  for(cpl=ObjCplList(context, hdr); cpl!=NULL; cpl=CPL_NEXT(cpl))
  {
    XIDelCpl *xc = NewXIDelCpl(context);
    if (xc==NULL)
      throw std::bad_alloc();

    xc->to      = CPL_PROC(cpl);                 /* receiver of XIDelCpl */
    xc->prio    = cpl->prio;                     /* remember priority    */
    xc->te.gid  = OBJ_GID(hdr);                  /* the object's gid     */

    /* we must remember couplings for eventual restoring
       (if this object is received from another proc) */
    xc->next = xi->delcpls;
    xi->delcpls = xc;
  }
}



/****************************************************************************/

/*
        management functions for XferMode.

        these functions control the mode the xfer-module is
        currently in. this is used for error detection, but
        also for correct detection of coupling inconsistencies
        and recovery.
 */

const char *XferModeName (enum XferMode mode)
{
  switch(mode)
  {
  case DDD::Xfer::XferMode::XMODE_IDLE : return "idle-mode";
  case DDD::Xfer::XferMode::XMODE_CMDS : return "commands-mode";
  case DDD::Xfer::XferMode::XMODE_BUSY : return "busy-mode";
  }
  return "unknown-mode";
}


static void XferSetMode(DDD::DDDContext& context, enum XferMode mode)
{
  auto& ctx = context.xferContext();

  ctx.xferMode = mode;

#       if DebugXfer<=8
  Dune::dwarn << "XferMode=" << XferModeName(ctx.xferMode) << "\n";
#       endif
}


static enum XferMode XferSuccMode (enum XferMode mode)
{
  switch(mode)
  {
  case DDD::Xfer::XferMode::XMODE_IDLE : return DDD::Xfer::XferMode::XMODE_CMDS;
  case DDD::Xfer::XferMode::XMODE_CMDS : return DDD::Xfer::XferMode::XMODE_BUSY;
  case DDD::Xfer::XferMode::XMODE_BUSY : return DDD::Xfer::XferMode::XMODE_IDLE;
  }
  DUNE_THROW(Dune::InvalidStateException, "invalid XferMode");
}



enum XferMode XferMode (const DDD::DDDContext& context)
{
  return context.xferContext().xferMode;
}


int ddd_XferActive(const DDD::DDDContext& context)
{
  return context.xferContext().xferMode != DDD::Xfer::XferMode::XMODE_IDLE;
}


int XferStepMode (DDD::DDDContext& context, enum XferMode old)
{
  const auto& ctx = context.xferContext();

  if (ctx.xferMode!=old)
  {
    Dune::dwarn
      << "wrong xfer-mode (currently in " << XferModeName(ctx.xferMode)
      << ", expected " << XferModeName(old) << ")\n";
    return false;
  }

  XferSetMode(context, XferSuccMode(ctx.xferMode));
  return true;
}


/****************************************************************************/


void ddd_XferInit(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();

  /* init control structures for XferInfo-items in first (?) message */
  ctx.setXICopyObj = reinterpret_cast<DDD::Xfer::XICopyObjSet*>(New_XICopyObjSet());
  reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj)->tree->context = &context;
  ctx.setXISetPrio = reinterpret_cast<DDD::Xfer::XISetPrioSet*>(New_XISetPrioSet());
  reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio)->tree->context = &context;
  InitXIDelCmd(context);
  InitXIDelObj(context);
  InitXINewCpl(context);
  InitXIOldCpl(context);

  /* init control structures for XferInfo-items for second (?) message */
  InitXIDelCpl(context);
  InitXIModCpl(context);
  InitXIAddCpl(context);


  XferSetMode(context, DDD::Xfer::XferMode::XMODE_IDLE);

  ctx.objmsg_t = LC_NewMsgType(context, "XferMsg");
  ctx.symtab_id = LC_NewMsgTable("SymTab",
                                 ctx.objmsg_t, sizeof(SYMTAB_ENTRY));
  ctx.objtab_id = LC_NewMsgTable("ObjTab",
                                 ctx.objmsg_t, sizeof(OBJTAB_ENTRY));
  ctx.newcpl_id = LC_NewMsgTable("NewCpl",
                                 ctx.objmsg_t, sizeof(TENewCpl));
  ctx.oldcpl_id = LC_NewMsgTable("OldCpl",
                                 ctx.objmsg_t, sizeof(TEOldCpl));
  ctx.objmem_id = LC_NewMsgChunk("ObjMem",
                                 ctx.objmsg_t);

  /* not used anymore
          ctx.deltab_id =
                  LC_NewMsgTable(ctx.objmsg_t, sizeof(DELTAB_ENTRY));
          ctx.priotab_id =
                  LC_NewMsgTable(ctx.objmsg_t, sizeof(CPLTAB_ENTRY));
   */

  CplMsgInit(context);
  CmdMsgInit(context);
}


void ddd_XferExit(DDD::DDDContext& context)
{
  auto& ctx = context.xferContext();

  CmdMsgExit(context);
  CplMsgExit(context);

  XICopyObjSet_Free(reinterpret_cast<XICopyObjSet*>(ctx.setXICopyObj));
  XISetPrioSet_Free(reinterpret_cast<XISetPrioSet*>(ctx.setXISetPrio));
}




END_UGDIM_NAMESPACE
