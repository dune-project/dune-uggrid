/****************************************************************************/
/*                                                                          */
/* File:      unpack.c                                                      */
/*                                                                          */
/* Purpose:   receives and unpacks messages                                 */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   940201 kb  begin                                              */
/*            960508 kb  restructured completely. efficiency improvement.   */
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
#include <cinttypes>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <algorithm>
#include <iomanip>
#include <tuple>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

#include <dune/uggrid/parallel/ddd/dddi.h>
#include "xfer.h"

USING_UG_NAMESPACE
using namespace PPIF;

START_UGDIM_NAMESPACE

/* TODO kb 961210
   #define DEBUG_MERGE_MODE
 */
#define MERGE_MODE_IN_TESTZUSTAND



/*#define DebugCouplingCons*/


/*
   #define AddCoupling(context, a,b,c)  printf("%4d: AC %05d, %d/%d     %08x\n",context.me(),__LINE__,b,c,(int) AddCoupling(context, a,b,c))
 */


static void NEW_AddCpl(DDD::DDDContext& context, DDD_PROC destproc, DDD_GID objgid, DDD_PROC cplproc, DDD_PRIO cplprio)
{
  XIAddCpl *xc = NewXIAddCpl(context);
  assert(xc);
  xc->to      = destproc;
  xc->te.gid  = objgid;
  xc->te.proc = cplproc;
  xc->te.prio = cplprio;
}


static bool sort_TENewCpl (const TENewCpl& a, const TENewCpl& b)
{
  /* sorting according to priority is not necessary anymore,
     equal items with different priorities will be sorted
     out according to PriorityMerge(). KB 970326
     if (ci1->prio < ci2->prio) return(-1);
     if (ci1->prio > ci2->prio) return(1);
   */
  return std::tie(a._gid, a._dest) < std::tie(b._gid, b._dest);
}


static bool sort_ObjTabPtrs (const OBJTAB_ENTRY* a, const OBJTAB_ENTRY* b)
{
  /* sort with ascending gid */
  /* sort with decreasing priority */
  /* not necessary anymore. see first phase of
     AcceptReceivedObjects() for details. KB 970128
     if (OBJ_PRIO(ci1) < OBJ_PRIO(ci2)) return(1);
     if (OBJ_PRIO(ci1) > OBJ_PRIO(ci2)) return(-1);
   */
  return OBJ_GID(a->hdr) < OBJ_GID(b->hdr);
}



/****************************************************************************/


/*
        convert indices to symtab into references (pointers).
        the object objmem gets all references from template msgmem.
        msgmem and objmem may point to the same storage (this feature is
        used by PutDepData() ).
 */

static void LocalizeObject (DDD::DDDContext& context, bool merge_mode, TYPE_DESC *desc,
                            const char *msgmem,
                            DDD_OBJ objmem,
                            const SYMTAB_ENTRY *theSymTab)
{
  ELEM_DESC     *theElem;
  int e;
  DDD_OBJ obj = objmem;

  /* prepare map of structure elements */
  theElem = desc->element;

  /* loop over all pointers inside of object obj */
  for(e=0; e<desc->nElements; e++, theElem++)
  {
    if (theElem->type==EL_OBJPTR)
    {
      TYPE_DESC *refdesc;
      int rt_on_the_fly = (EDESC_REFTYPE(theElem)==DDD_TYPE_BY_HANDLER);
      int l;

      const char      *msgrefarray = msgmem+theElem->offset;
      char      *objrefarray = objmem+theElem->offset;


      /* determine reftype of this elem */
      if (! rt_on_the_fly)
      {
        refdesc = &context.typeDefs()[EDESC_REFTYPE(theElem)];
      }
      /* else determine reftype on the fly */



      /* loop over single pointer array */
      for(l=0; l<theElem->size; l+=sizeof(void*))
      {
        INT stIdx;

        /* ref points to a reference inside objmem */
        DDD_OBJ *ref = (DDD_OBJ *) (objrefarray+l);


        /* reference had been replaced by SymTab-index */
        stIdx = (*(std::uintptr_t *)(msgrefarray+l)) - 1;
        /* test for Localize execution in merge_mode */
        if (merge_mode && (*ref!=NULL))
        {
          if (rt_on_the_fly)
          {
            /* determine reftype on the fly by calling handler */
            DDD_TYPE rt;

            assert(obj!=NULL);

            rt = theElem->reftypeHandler(context, obj, *ref);

            if (rt>=MAX_TYPEDESC)
              DUNE_THROW(Dune::Exception,
                         "invalid referenced DDD_TYPE returned by handler");

            refdesc = &context.typeDefs()[rt];
          }

          /* if we are in merge_mode, we do not update
             existing references. */
                                        #ifdef DEBUG_MERGE_MODE
          printf("loc-merge curr=%08x keep     e=%d l=%d\n",
                 OBJ_GID(OBJ2HDR(*ref,refdesc)), e,l);
                                        #endif

          /* it may happen here that different references
             are in incoming and existing object. this is implicitly
             resolved by using the existing reference and ignoring
             the incoming one. if the REF_COLLISION option is set,
                  we will issue a warning.
           */

          if (stIdx>=0 &&
              DDD_GetOption(context, OPT_WARNING_REF_COLLISION)==OPT_ON)
          {
            /* get corresponding symtab entry */
            if (theSymTab[stIdx].adr.hdr!=OBJ2HDR(*ref,refdesc))
            {
              Dune::dwarn
                << "LocalizeObject: "
                << "reference collision in " << OBJ_GID(OBJ2HDR(obj,desc))
                << " (old=" << OBJ_GID(OBJ2HDR(*ref,refdesc))
                << ", inc=" << OBJ_GID(theSymTab[stIdx].adr.hdr) << ")\n";
              /* assert(0);  ??? */
            }
          }

          continue;
        }


        /*
           at this point, we are either not in merge_mode
           or we are in merge_mode, but existing reference is zero.

           NOTE: only in merge_mode the objmem array points
                 to the reference array inside the local
                 local object!
         */

        if (stIdx>=0)
        {
          /* get corresponding symtab entry */
          const SYMTAB_ENTRY *st = &(theSymTab[stIdx]);

          /*
                  convert reference from header to object itself
                  and replace index by pointer; if header==NULL,
                  referenced object is not known and *ref should
                  therefore be NULL, too!
           */

#ifdef MERGE_MODE_IN_TESTZUSTAND
          if (merge_mode)
          {
            if (st->adr.hdr!=NULL)
            {
                                                        #ifdef DEBUG_MERGE_MODE
              printf("loc-merge curr=%08x "
                     "have_sym e=%d l=%d to %08x\n",
                     *ref, e,l,OBJ_GID(st->adr.hdr));
                                                        #endif

              /* distinction for efficiency: if we know refdesc
                 in advance, we can compute DDD_OBJ more
                 efficient. */
              if (!rt_on_the_fly)
              {
                *ref = HDR2OBJ(st->adr.hdr,refdesc);
              }
              else
              {
                *ref = OBJ_OBJ(context, st->adr.hdr);
              }
            }

                                                #ifdef DEBUG_MERGE_MODE
            else
            {
              printf(
                "loc-merge curr=%08x "
                "have_sym e=%d l=%d to NULL\n",
                *ref, e, l);
            }
                                                #endif
          }
          else
#endif
          {
            if (st->adr.hdr!=NULL)
            {
              /* distinction for efficiency: if we know refdesc
                 in advance, we can compute DDD_OBJ more
                 efficient. */
              if (!rt_on_the_fly)
              {
                *ref = HDR2OBJ(st->adr.hdr,refdesc);
              }
              else
              {
                *ref = OBJ_OBJ(context, st->adr.hdr);
              }
            }
            else
              *ref = NULL;
          }
        }
        else
        {
#ifdef MERGE_MODE_IN_TESTZUSTAND
          if (merge_mode)
          {
                                                #ifdef DEBUG_MERGE_MODE
            printf("loc-merge curr=%08x "
                   "no_sym   e=%d l=%d\n",
                   *ref, e,l);
                                                #endif
          }
          else
#endif
          {
            *ref = NULL;
          }
        }
      }
    }
  }
}



/****************************************************************************/



static void PutDepData (DDD::DDDContext& context,
                        char *data,
                        const TYPE_DESC *desc,
                        DDD_OBJ obj,
                        const SYMTAB_ENTRY *theSymTab,
                        int newness)
{
  /* get overall number of chunks */
  const int chunks = ((int *)data)[0];
  char* chunk = data + CEIL(sizeof(int));

  char* curr = nullptr;

  /* loop through all chunks */
  for(int j=0; j<chunks; j++)
  {
    /* first entries of chunk are addCnt and addTyp */
    int addCnt = ((int *)chunk)[0];
    DDD_TYPE addTyp = ((DDD_TYPE *)chunk)[1];
    chunk += CEIL(sizeof(int)+sizeof(DDD_TYPE));

    if (addCnt>=0)
    {
      if (addTyp<DDD_USER_DATA || addTyp>DDD_USER_DATA_MAX)
      {
        /* convert pointers using SymTab */
        TYPE_DESC* descDep = &context.typeDefs()[addTyp];
        curr = chunk;
        for(int i=0; i<addCnt; i++)
        {
          /* insert pointers into copy using SymTab */
          if (descDep->nPointers>0)
          {
            LocalizeObject(context, false, descDep,
                           curr,
                           (DDD_OBJ)curr,
                           theSymTab);
          }
          curr += CEIL(descDep->size);
        }
      }
      else
      {
        /* addType>=DDD_USER_DATA && addType <= DDD_USER_DATA_MAX ->
              scatter stream of bytes with len addCnt */
        curr = chunk + CEIL(addCnt);
      }

      /* scatter data via handler */
      if (desc->handlerXFERSCATTER)
      {
        desc->handlerXFERSCATTER(context, obj, addCnt, addTyp, (void *)chunk, newness);
      }
    }
    else
    {
      /* variable sized chunks */
      addCnt *= -1;

      /* convert offset table into pointer table */
      TYPE_DESC* descDep = &context.typeDefs()[addTyp];
      char** table = (char **)chunk;
      chunk += CEIL(sizeof(int)*addCnt);
      char* adr = chunk;
      for(int i=0; i<addCnt; i++)
      {
        table[i] = ((long int)table[i])+adr;

        /* insert pointers into copy using SymTab */
        if (addTyp<DDD_USER_DATA || addTyp>DDD_USER_DATA_MAX)
        {
          curr = table[i];
          if (descDep->nPointers>0)
            LocalizeObject(context, false, descDep,
                           curr,
                           (DDD_OBJ)curr,
                           theSymTab);
        }
      }

      /* scatter data via handler */
      if (desc->handlerXFERSCATTERX)
      {
        desc->handlerXFERSCATTERX(context, obj, addCnt, addTyp, table, newness);
      }
    }

    chunk = curr;
  }
}




/****************************************************************************/


static void AcceptObjFromMsg (
  DDD::DDDContext& context,
  OBJTAB_ENTRY *theObjTab, int lenObjTab,
  const char *theObjects,
  const DDD_HDR *localCplObjs, int nLocalCplObjs)
{
  int i, j;

  for(i=0, j=0; i<lenObjTab; i++)
  {
    OBJTAB_ENTRY *ote = &theObjTab[i];
    TYPE_DESC    *desc = &context.typeDefs()[OBJ_TYPE(ote->hdr)];

    if (ote->is_new == OTHERMSG)
    {
      /* object is in another message with higher priority */
      continue;
    }

    while ((j<nLocalCplObjs) && (OBJ_GID(localCplObjs[j]) < OBJ_GID(ote->hdr)))
      j++;

    if ((j<nLocalCplObjs) && (OBJ_GID(localCplObjs[j])==OBJ_GID(ote->hdr)))
    {
      /* object already here, compare priorities.
         this is the implementation of rule XFER-C3. */
      DDD_PRIO newprio;
      enum PrioMergeVals ret;

      /* if local object should have been XferDelete'd, but the
         delete-cmd had been pruned (see cmdmsg.c), a flag has been
         set in its header. we have to ensure here that all incoming
         objects win against pruned-deleted ones, because the object
         serves only as 'cache' for data (esp. pointers) */
      if (OBJ_PRUNED(localCplObjs[j]))
      {
#                               if DebugUnpack<=1
        Dune::dvverb << "NewPrio wins due to PruneDel. "
                     << OBJ_GID(ote->hdr) << "\n";
#                               endif

        /* reset flag */
        SET_OBJ_PRUNED(localCplObjs[j], 0);

        /* simply copy new priority, disregard old one */
        newprio = OBJ_PRIO(ote->hdr);

        ote->is_new = PRUNEDNEW;
      }
      else
      {
        ret = PriorityMerge(desc,
                            OBJ_PRIO(ote->hdr), OBJ_PRIO(localCplObjs[j]), &newprio);

        if (ret==PRIO_FIRST || ret==PRIO_UNKNOWN)                          /* incoming is higher or equal */
        {
          DDD_OBJ copy;

#                               if DebugUnpack<=1
          Dune::dvverb << "NewPrio wins. " << OBJ_GID(ote->hdr) << "\n";
#                               endif

          /* new priority wins -> recreate */
          /* all GDATA-parts are overwritten by contents of message */
          copy = OTE_OBJ(context, theObjects,ote);
          ObjCopyGlobalData(desc,
                            HDR2OBJ(localCplObjs[j],desc), copy, ote->size);

          ote->is_new = PARTNEW;
        }
        else                          /* existing is higher than incoming */
        {
#                                       if DebugUnpack<=1
          Dune::dvverb << "OldPrio wins. " << OBJ_GID(ote->hdr) << "\n";
#                                       endif

          /* new priority loses -> keep existing obj */
          ote->is_new = NOTNEW;
        }
      }

      /* store pointer to local object */
      /* overwrite pointer to hdr inside message */
      if (OBJ_TYPE(ote->hdr) != OBJ_TYPE(localCplObjs[j]))
      {
        printf("ERROR, copying changed the object type!\n");
        printf("    was: %s, becomes: %s\n",
               context.typeDefs()[OBJ_TYPE(ote->hdr)].name,
               context.typeDefs()[OBJ_TYPE(localCplObjs[j])].name);
        assert(OBJ_TYPE(ote->hdr) == OBJ_TYPE(localCplObjs[j]));
      }
      ote->hdr = localCplObjs[j];

      /* store old priority and set new one */
      OTE_PRIO(theObjects,ote) = newprio;
      ote->oldprio             = OBJ_PRIO(localCplObjs[j]);

      /* the next line is not useful. the current priority
         of the involved object will be changed to newprio-value
         after calling of handler SETPRIORITY. KB 970417 */
      /* OBJ_PRIO(localCplObjs[j]) = newprio; */
    }
    else
    {
      DDD_OBJ msgcopy, newcopy;
      DDD_PRIO new_prio = OBJ_PRIO(ote->hdr);

#                       if DebugUnpack<=1
      Dune::dvverb << "NewObject       " << OBJ_GID(ote->hdr)
                   << ", prio=" << new_prio << "\n";
#                       endif

      /* new object, create local copy */
      msgcopy = OTE_OBJ(context, theObjects,ote);
      newcopy = DDD_ObjNew(ote->size,
                           OBJ_TYPE(ote->hdr), new_prio, OBJ_ATTR(ote->hdr));

      /* overwrite pointer to hdr inside message */
      ote->hdr = OBJ2HDR(newcopy,desc);

      /* copy GDATA */
      ObjCopyGlobalData(desc, newcopy, msgcopy, ote->size);
      ote->is_new = TOTALNEW;

      /* construct HDR */
      DDD_HdrConstructorCopy(context, ote->hdr, new_prio);

      /* construct LDATA */
      if (desc->handlerLDATACONSTRUCTOR)
        desc->handlerLDATACONSTRUCTOR(context, newcopy);
    }
  }
}



static void AcceptReceivedObjects (DDD::DDDContext& context,
                                   const LC_MSGHANDLE *theMsgs, int nRecvMsgs,
                                   OBJTAB_ENTRY **allRecObjs, int nRecObjs,
                                   const DDD_HDR *localCplObjs, int nLocalCplObjs)
{
  /*
          allRecObjs is a pointer array to all OBJTAB_ENTRYs
          received in incoming messages. it is sorted according
          to (gid/ascending).

          1. collision detection for incoming objects with same
                  gid: accept object with merged priority. if several
                  such objects exist, an arbitrary one is chosen.
                  discard all other objects with same gid. (RULE XFER-C2)

          2. transfer objects from message into local memory.

          3. propagate hdr-pointer to all OBJTAB_ENTRYs with equal gid.
   */

  auto& ctx = context.xferContext();
  int i;

  if (nRecObjs==0)
    return;

  /* 1. collision detection */
  for(i=nRecObjs-1; i>0; i--)
  {
    if (OBJ_GID(allRecObjs[i]->hdr) != OBJ_GID(allRecObjs[i-1]->hdr))
    {
      allRecObjs[i]->is_new = THISMSG;
    }
    else
    {
      DDD_PRIO newprio;
      int ret;

      ret = PriorityMerge(&context.typeDefs()[OBJ_TYPE(allRecObjs[i]->hdr)],
                          OBJ_PRIO(allRecObjs[i]->hdr), OBJ_PRIO(allRecObjs[i-1]->hdr), &newprio);

      if (ret==PRIO_FIRST || ret==PRIO_UNKNOWN)
      {
        /* item i is winner */
        OBJTAB_ENTRY *tmp;

        OBJ_PRIO(allRecObjs[i]->hdr) = newprio;

        /* switch first item i in second position i-1 */
        /* item on first position will be discarded */
        tmp = allRecObjs[i];
        allRecObjs[i] = allRecObjs[i-1];
        allRecObjs[i-1] = tmp;
      }
      else
      {
        /* item i-1 is winner */
        OBJ_PRIO(allRecObjs[i-1]->hdr) = newprio;
      }

      /* mark item i invalid */
      allRecObjs[i]->is_new = OTHERMSG;
    }
  }
  allRecObjs[0]->is_new = THISMSG;

  /* now the first item in a series of items with equal gid is the
     THISMSG-item, the following are OTHERMSG-items */


  /* 2. transfer from message into local memory */
  for(i=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];

    AcceptObjFromMsg(
      context,
      (OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id),
      (int)    LC_GetTableLen(xm, ctx.objtab_id),
      (char *) LC_GetPtr(xm, ctx.objmem_id),
      localCplObjs, nLocalCplObjs
      );
  }


  /* 3. propagate hdr-pointer */
  for(i=1; i<nRecObjs; i++)
  {
    if (allRecObjs[i]->is_new == OTHERMSG)
    {
      /* propagate hdr-pointer */
      allRecObjs[i]->hdr = allRecObjs[i-1]->hdr;
    }
  }
}



/****************************************************************************/


static void AddAndSpread (DDD::DDDContext& context,
                          DDD_HDR hdr, DDD_GID gid, DDD_PROC dest, DDD_PRIO prio,
                          XICopyObj **itemsNO, int nNO)
{
  int k;

  if (hdr!=NULL)
    AddCoupling(context, hdr, dest, prio);

  for(k=0; k<nNO; k++)
  {
    if (itemsNO[k]->dest != dest)
      NEW_AddCpl(context, itemsNO[k]->dest, gid, dest, prio);
  }
}


/*
        this function updates the couplings of local objects.
        the inputs for deciding which couplings have to be added are:

        for prev. existing objects:
                -  sending to NEWOWNER-destinations
                -  incoming NewCpl-items for previously existing objects

        for new (incoming) objects:
                -  incoming NewCpl-items for new objects

        as a side effect this function sends XIAddCpl-items
        to NEWOWNER-procs.
 */


enum UpdateCpl_Cases { UCC_NONE, UCC_NO, UCC_NC, UCC_NO_AND_NC };


static void UpdateCouplings (
  DDD::DDDContext& context,
  TENewCpl *itemsNC, int nNC,                     /* NewCpl  */
  OBJTAB_ENTRY **itemsO, int nO,                  /* Objects */
  const DDD_HDR *itemsLCO, int nLCO,                                /* local objs with coupling */
  XIDelObj  **itemsDO, int nDO,                   /* XIDelObj */
  XICopyObj **itemsNO, int nNO)                   /* NewOwners */
{
  const auto& me = context.me();
  const auto& procs = context.procs();
  int iNC, iO, iDO, iNO, iLCO;

  /*
          each NewCpl either corresponds to an incoming object
          or to a local object, but not both.
   */

  /*** loop for all incoming objects ***/
  for(iO=0, iNC=0, iDO=0; iO<nO; iO++)
  {
    DDD_HDR hdr = itemsO[iO]->hdr;
    DDD_GID gid = OBJ_GID(hdr);

    /* scan DelObj-entries for given gid */
    while (iDO<nDO && itemsDO[iDO]->gid < gid)
      iDO++;


    /* if there is a DelObj-item with same gid, then the object
       has been deleted and send by a remote proc afterwards.
       we must:
         - restore old couplings locally
             - invalidate XIDelCpl-items
     */
    if (iDO<nDO && itemsDO[iDO]->gid == gid)
    {
      XIDelCpl *dc = itemsDO[iDO]->delcpls;
      for( ; dc!=NULL; dc=dc->next)
      {
        /* restore previous coupling */
        if (dc->prio!=PRIO_INVALID)
          AddCoupling(context, hdr, dc->to, dc->prio);

        /* invalidate XIDelCpl-item */
        dc->to=procs;
      }

      /* restore only one time */
      itemsDO[iDO]->delcpls = NULL;
    }


    /* scan NewCpl-entries for given gid */
    while (iNC<nNC && NewCpl_GetGid(itemsNC[iNC]) < gid)
      iNC++;

    /* for all NewCpl-Items with same gid as incoming object */
    while (iNC<nNC && NewCpl_GetGid(itemsNC[iNC]) == gid)
    {
      /* there is a corresponding NewCpl-item */
      AddCoupling(context, hdr,
                  NewCpl_GetDest(itemsNC[iNC]),
                  NewCpl_GetPrio(itemsNC[iNC]));
      NEW_AddCpl(context, NewCpl_GetDest(itemsNC[iNC]), gid, me, OBJ_PRIO(hdr));

      iNC++;
    }
  }



  /*** loop for previously existing objects ***/
  iNO=iNC=iLCO=iDO=iO=0;
  while (iNO<nNO || iNC<nNC)
  {
    DDD_HDR hdrNO, hdrNC;
    DDD_GID gidNO, gidNC;
    int moreNO, moreNC, curr_case;
    int firstNC,lastNC;
    int firstNO,lastNO,nNOset;
    XICopyObj **setNO;

    /* scan all NewOwner-items with same gid, and set first/last indices */
    firstNO = iNO;
    while (iNO<nNO-1 && itemsNO[iNO+1]->gid==itemsNO[iNO]->gid)
      iNO++;
    lastNO = iNO;
    nNOset = 1+lastNO-firstNO;
    setNO = itemsNO+firstNO;

    /* scan all NewCpl-items with same gid, and set first/last indices */
    firstNC = iNC;
    while (iNC<nNC-1 && NewCpl_GetGid(itemsNC[iNC+1])==NewCpl_GetGid(itemsNC[iNC]))
      iNC++;
    lastNC = iNC;

    /*
       printf("%4d: MULTILOOP  NewOwner %3d-%3d of %3d  NewCpl %3d-%3d of %3d\n",
            me, firstNO, lastNO, nNO, firstNC, lastNC, nNC);
     */

    /* set control flags */
    moreNO = (iNO<nNO); if (moreNO) gidNO = setNO[0]->gid;
    moreNC = (iNC<nNC); if (moreNC) gidNC = NewCpl_GetGid(itemsNC[firstNC]);

    curr_case = UCC_NONE;
    if (moreNO && (!moreNC || gidNO<gidNC))
      curr_case = UCC_NO;
    if (moreNC && (!moreNO || gidNC<gidNO))
      curr_case = UCC_NC;
    if (moreNO && moreNC && gidNC==gidNO)
      curr_case = UCC_NO_AND_NC;


    /* find DDD_HDR for given gid */
    hdrNO = hdrNC = NULL;
    if (moreNC)
    {
      /* scan local objects with couplings */
      while (iLCO<nLCO && OBJ_GID(itemsLCO[iLCO])<gidNC) iLCO++;
      if (iLCO<nLCO && OBJ_GID(itemsLCO[iLCO])==gidNC)
        hdrNC = itemsLCO[iLCO];
    }
    if (moreNO)
    {
      /* check whether obj has been deleted during this xfer */
      while (iDO<nDO && itemsDO[iDO]->gid < gidNO) iDO++;
      if (! (iDO<nDO && itemsDO[iDO]->gid==gidNO))
      {
        /* there is no DelObj-item */
        hdrNO = setNO[0]->hdr;
      }

      /* scan received objects */
      while (iO<nO && OBJ_GID(itemsO[iO]->hdr) < gidNO) iO++;
      if (iO<nO && OBJ_GID(itemsO[iO]->hdr)==gidNO)
      {
        /* obj has been deleted and received again */
        assert(hdrNO==NULL || hdrNO==itemsO[iO]->hdr);
        hdrNO = itemsO[iO]->hdr;
      }
    }


    switch (curr_case)
    {
    case UCC_NONE :                          /* no other case is valid -> do nothing */
      break;


    case UCC_NO :                            /* there is a NewOwner set without NewCpl set */
    {
      int jNO;

      for(jNO=0; jNO<nNOset; jNO++)
      {
        /* there is no NewCpl-item for given dest */
        AddAndSpread(context, hdrNO, gidNO, setNO[jNO]->dest,
                     setNO[jNO]->prio, setNO, nNOset);
      }

      /* step to next gid */
      iNO = lastNO+1;
      iNC = firstNC;
    }
    break;


    case UCC_NC :                            /* there is a NewCpl set without NewOwner set */
    {
      int jNC;

      for(jNC=firstNC; jNC<=lastNC; jNC++)
      {
        if (hdrNC!=NULL)
          AddCoupling(context, hdrNC,
                      NewCpl_GetDest(itemsNC[jNC]),
                      NewCpl_GetPrio(itemsNC[jNC]));
        /* else: dont need to AddCpl to deleted object */
      }

      /* step to next gid */
      iNC = lastNC+1;
      iNO = firstNO;
    }
    break;


    case UCC_NO_AND_NC :                     /* there are both NewCpl and NewOwner sets */
    {
      DDD_HDR hdr;
      int jNO, jNC;

      /* same gids -> same object and header */
      assert(hdrNO==NULL || hdrNC==NULL || hdrNO==hdrNC);

      if (hdrNO==NULL) hdr = hdrNC;
      else hdr = hdrNO;

      jNC = firstNC;
      for(jNO=0; jNO<nNOset; jNO++)
      {
        /* scan NewCpl-items for given dest processor */
        while (jNC<=lastNC && NewCpl_GetDest(itemsNC[jNC]) < setNO[jNO]->dest)
        {
          AddAndSpread(context, hdr, gidNO,
                       NewCpl_GetDest(itemsNC[jNC]),
                       NewCpl_GetPrio(itemsNC[jNC]),
                       setNO, nNOset);
          jNC++;
        }

        if (jNC<=lastNC && NewCpl_GetDest(itemsNC[jNC]) == setNO[jNO]->dest)
        {
          /* found NewCpl-item */
          DDD_PRIO newprio;

          PriorityMerge(&context.typeDefs()[NewCpl_GetType(itemsNC[jNC])],
                        setNO[jNO]->prio, NewCpl_GetPrio(itemsNC[jNC]), &newprio);

          AddAndSpread(context, hdr, gidNO, setNO[jNO]->dest, newprio,
                       setNO, nNOset);
          jNC++;
        }
        else
        {
          /* there is no NewCpl-item for given dest */
          AddAndSpread(context, hdr, gidNO, setNO[jNO]->dest, setNO[jNO]->prio,
                       setNO, nNOset);
        }
      }
      while (jNC<=lastNC)
      {
        AddAndSpread(context, hdr, gidNO,
                     NewCpl_GetDest(itemsNC[jNC]),
                     NewCpl_GetPrio(itemsNC[jNC]),
                     setNO, nNOset);
        jNC++;
      }

      /* step to next gid */
      iNO = lastNO+1;
      iNC = lastNC+1;
    }
    break;

    default :
      assert(0);
      break;
    }
  }
}




/*
        this function handles local objects, which had been here before
        xfer. during xfer, another object with same gid was received
        and lead to a higher priority. this priority change must
        be communicated to all destination-procs, to which the local
   processor sent a copy during xfer.
 */
static void PropagateIncomings (
  DDD::DDDContext& context,
  XICopyObj **arrayNO, int nNO,
  OBJTAB_ENTRY **allRecObjs, int nRecObjs)
{
  int iRO, iNO;

  for(iRO=0, iNO=0; iRO<nRecObjs; iRO++)
  {
    int newness = allRecObjs[iRO]->is_new;

    if (newness==PARTNEW || newness==PRUNEDNEW || newness==TOTALNEW)
    {
      COUPLING *cpl;

      /* object has been local before, but changed its prio */
      OBJTAB_ENTRY *ote = allRecObjs[iRO];

      /* scan received objects */
      while ((iNO<nNO) && (arrayNO[iNO]->gid < OBJ_GID(ote->hdr)))
        iNO++;

      /* communicate to all NEWOWNER-destinations */
      while (iNO<nNO && arrayNO[iNO]->gid == OBJ_GID(ote->hdr))
      {
        if (newness==PARTNEW || newness==PRUNEDNEW)
        {
          XIModCpl *xc = NewXIModCpl(context);
          if (xc==NULL)
            throw std::bad_alloc();

          xc->to      = arrayNO[iNO]->dest;                               /* receiver of XIModCpl*/
          xc->te.gid  = OBJ_GID(ote->hdr);                                /* the object's gid    */
          xc->te.prio = OBJ_PRIO(ote->hdr);                               /* the obj's new prio  */
          xc->typ     = OBJ_TYPE(ote->hdr);                               /* the obj's ddd-type  */
        }

        iNO++;
      }

      /* communicate to all procs in coupling */
      for(cpl=ObjCplList(context, ote->hdr); cpl!=NULL; cpl=CPL_NEXT(cpl))
      {
        /*
                                        if (newness==PARTNEW || newness==PRUNEDNEW)
                                        {
         */
        XIModCpl *xc = NewXIModCpl(context);
        if (xc==NULL)
          throw std::bad_alloc();

        xc->to      = CPL_PROC(cpl);                                      /* receiver of XIModCpl*/
        xc->te.gid  = OBJ_GID(ote->hdr);                                  /* the object's gid   */
        xc->te.prio = OBJ_PRIO(ote->hdr);                                 /* the obj's new prio */
        xc->typ     = OBJ_TYPE(ote->hdr);                                 /* the obj's ddd-type  */
        /*
                                        }
         */
      }
    }
  }
}


/****************************************************************************/



static void LocalizeSymTab (DDD::DDDContext& context, LC_MSGHANDLE xm,
                            OBJTAB_ENTRY **allRecObjs, int nRecObjs,
                            const DDD_HDR *localCplObjs, int nLocalCplObjs)
{
  auto& ctx = context.xferContext();

  SYMTAB_ENTRY *theSymTab;
  int i, j;
  int lenSymTab = (int) LC_GetTableLen(xm, ctx.symtab_id);


  /* get table addresses inside message buffer */
  theSymTab = (SYMTAB_ENTRY *) LC_GetPtr(xm, ctx.symtab_id);


  /* insert pointers to known objects into SymTab */
  for(i=0, j=0; i<lenSymTab; i++)
  {
    while ((j<nLocalCplObjs) &&
           (OBJ_GID(localCplObjs[j]) < theSymTab[i].gid))
      j++;

    if (j==nLocalCplObjs)
    {
      /* no more valid local objects */
      theSymTab[i].adr.hdr = NULL;
    }
    else
    {
      if (OBJ_GID(localCplObjs[j]) == theSymTab[i].gid)
      {
        theSymTab[i].adr.hdr = localCplObjs[j];
      }
      else
      {
        theSymTab[i].adr.hdr = NULL;
      }
    }
  }


  /* insert new pointers in SymTab */
  for(i=0, j=0; i<lenSymTab; i++)
  {
    while ((j<nRecObjs) && (OBJ_GID(allRecObjs[j]->hdr)<theSymTab[i].gid))
      j++;

    if ((j<nRecObjs) && (OBJ_GID(allRecObjs[j]->hdr)==theSymTab[i].gid))
    {
      theSymTab[i].adr.hdr = allRecObjs[j]->hdr;
    }
  }
}


static void LocalizeObjects (DDD::DDDContext& context, LC_MSGHANDLE xm, bool required_newness)
{
  auto& ctx = context.xferContext();
  const SYMTAB_ENTRY *theSymTab;
  const OBJTAB_ENTRY *theObjTab;
  const char         *theObjects;
  int i;
  int lenObjTab = (int) LC_GetTableLen(xm, ctx.objtab_id);


  /* get table addresses inside message buffer */
  theSymTab = (const SYMTAB_ENTRY *) LC_GetPtr(xm, ctx.symtab_id);
  theObjTab = (const OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id);
  theObjects = (const char *)        LC_GetPtr(xm, ctx.objmem_id);


  for(i=0; i<lenObjTab; i++)               /* for all message items */
  {
    if (required_newness && theObjTab[i].is_new==TOTALNEW)
    {
      TYPE_DESC *desc = &context.typeDefs()[OBJ_TYPE(theObjTab[i].hdr)];
      DDD_OBJ obj   = HDR2OBJ(theObjTab[i].hdr, desc);

      if (desc->nPointers>0)
      {
        LocalizeObject(context, false, desc,
                       (char *)(OTE_OBJ(context, theObjects,&(theObjTab[i]))),
                       obj,
                       theSymTab);
      }
    }


                #ifdef MERGE_MODE_IN_TESTZUSTAND

    if (required_newness && theObjTab[i].is_new!=TOTALNEW)
    {
      /*
              implemented merge_mode for Localize. references from all copies
              will be merged into the local copy. 960813 KB
       */
      TYPE_DESC *desc = &context.typeDefs()[OBJ_TYPE(theObjTab[i].hdr)];
      DDD_OBJ obj   = HDR2OBJ(theObjTab[i].hdr, desc);

      if (desc->nPointers>0)
      {
                                #ifdef DEBUG_MERGE_MODE
        printf("LocalizeObject in merge_mode, %08x prio %d\n",
               OBJ_GID(theObjTab[i].hdr), OBJ_PRIO(theObjTab[i].hdr));
                                #endif

        /* execute Localize in merge_mode */
        LocalizeObject(context, true, desc,
                       (char *)(OTE_OBJ(context, theObjects,&(theObjTab[i]))),
                       obj,
                       theSymTab);
      }
    }

                #endif
  }
}



static void CallUpdateHandler (DDD::DDDContext& context, LC_MSGHANDLE xm)
{
  auto& ctx = context.xferContext();
  OBJTAB_ENTRY *theObjTab;
  int lenObjTab = (int) LC_GetTableLen(xm, ctx.objtab_id);
  int i;

  /* get table addresses inside message buffer */
  theObjTab = (OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id);

  /* initialize new objects corresponding to application: update */
  for(i=0; i<lenObjTab; i++)               /* for all message items */
  {
    if (theObjTab[i].is_new == TOTALNEW)
    {
      const TYPE_DESC& desc = context.typeDefs()[OBJ_TYPE(theObjTab[i].hdr)];

      /* call application handler for object updating */
      if (desc.handlerUPDATE)
      {
        DDD_OBJ obj   = HDR2OBJ(theObjTab[i].hdr, &desc);
        desc.handlerUPDATE(context, obj);
      }
    }
  }
}



static void UnpackAddData (DDD::DDDContext& context, LC_MSGHANDLE xm, bool required_newness)
{
  auto& ctx = context.xferContext();
  SYMTAB_ENTRY *theSymTab;
  OBJTAB_ENTRY *theObjTab;
  char         *theObjects;
  int i;
  int lenObjTab = (int) LC_GetTableLen(xm, ctx.objtab_id);


  /* get table addresses inside message buffer */
  theSymTab = (SYMTAB_ENTRY *) LC_GetPtr(xm, ctx.symtab_id);
  theObjTab = (OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id);
  theObjects = (char *)        LC_GetPtr(xm, ctx.objmem_id);


  /* scatter additional data via handler */
  for(i=0; i<lenObjTab; i++)               /* for all message items */
  {
    if (theObjTab[i].addLen>0)
    {
      int newness = -1;

      if (required_newness)
      {
        if (theObjTab[i].is_new==TOTALNEW)
        {
          newness = XFER_NEW;
        }
      }
      else
      {
        if (theObjTab[i].is_new!=TOTALNEW)
        {
          switch (theObjTab[i].is_new)
          {
          case OTHERMSG :   newness=XFER_REJECT;   break;
          case NOTNEW :     newness=XFER_REJECT;   break;
          case PARTNEW :    newness=XFER_UPGRADE;  break;
          case PRUNEDNEW :  newness=XFER_UPGRADE;  break;
            /* TODO: for PRUNEDNEW we should merge prios; might be XFER_DOWNGRADE... */
          }
        }
      }

      if (newness!=-1)
      {
        TYPE_DESC *desc = &context.typeDefs()[OBJ_TYPE(theObjTab[i].hdr)];
        DDD_OBJ obj   = HDR2OBJ(theObjTab[i].hdr, desc);
        char      *data;

        /*
                compute begin of data section. theObjTab[i].size is equal to
                desc->len for fixed sized objects and different for variable
                sized objects
         */
        data = (char *)(OTE_OBJ(context, theObjects,&(theObjTab[i])) +
                        CEIL(theObjTab[i].size));

        PutDepData(context, data, desc, obj, theSymTab, newness);
      }
    }
  }
}



/*
        in order to allow application reactions on a priority
        change, the SETPRIORITY-handler is called.

        TODO: is this really a reason for calling SETPRIORITY? or
        should there be a separate handler for this task?

        NOTE: due to the current implementation, the new priority
        has already been set in the local object's DDD_HEADER.
        but the SETPRIORITY-handler has to get the old priority inside
        the object and the new one as second argument. so we restore
        the old prio before calling the handler and set the newprio
        afterwards.
 */
static void CallSetPriorityHandler (DDD::DDDContext& context, LC_MSGHANDLE xm)
{
  auto& ctx = context.xferContext();
  OBJTAB_ENTRY *theObjTab;
  int lenObjTab = (int) LC_GetTableLen(xm, ctx.objtab_id);
  int i;
  char         *theObjects;

  /* get table addresses inside message buffer */
  theObjTab  = (OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id);
  theObjects = (char *)         LC_GetPtr(xm, ctx.objmem_id);

  for(i=0; i<lenObjTab; i++)               /* for all message items */
  {
    /*
            the next condition is crucial. the SETPRIORITY-handler is
            called for _all_ object collisions, even if the old priority
            and the new priority are equal! this is to give the user
            a chance to note object collisions at all. (BTW, he could
            note it also during handlerMKCONS, but this could be too
            late...)
            970410 kb
     */
    if ((theObjTab[i].is_new==NOTNEW ||
         theObjTab[i].is_new==PARTNEW ||
         theObjTab[i].is_new==PRUNEDNEW)
        /*	&& (theObjTab[i].oldprio != theObjTab[i].prio) */)
    {
      const TYPE_DESC& desc = context.typeDefs()[OBJ_TYPE(theObjTab[i].hdr)];

      /* call application handler for object consistency */
      if (desc.handlerSETPRIORITY)
      {
        /* restore old priority in object */
        DDD_OBJ obj   = HDR2OBJ(theObjTab[i].hdr, &desc);
        DDD_PRIO new_prio = OTE_PRIO(theObjects, &(theObjTab[i]));                        /* remember new prio */
        OBJ_PRIO(theObjTab[i].hdr) = theObjTab[i].oldprio;

        desc.handlerSETPRIORITY(context, obj, new_prio);

        /* restore new priority */
        OBJ_PRIO(theObjTab[i].hdr) = new_prio;
      }
    }
  }
}



static void CallObjMkConsHandler (DDD::DDDContext& context, LC_MSGHANDLE xm, bool required_newness)
{
  auto& ctx = context.xferContext();
  OBJTAB_ENTRY *theObjTab;
  int lenObjTab = (int) LC_GetTableLen(xm, ctx.objtab_id);
  int i;

  /*STAT_RESET4;*/

  /* get table addresses inside message buffer */
  theObjTab = (OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id);


  /* initialize new objects corresponding to application: consistency */
  for(i=0; i<lenObjTab; i++)               /* for all message items */
  {
    int newness = -1;

    if (required_newness)
    {
      if (theObjTab[i].is_new==TOTALNEW)
      {
        newness = XFER_NEW;
      }
    }
    else
    {
      switch (theObjTab[i].is_new)
      {
      case NOTNEW :    newness=XFER_REJECT;   break;
      case PARTNEW :   newness=XFER_UPGRADE;  break;
      case PRUNEDNEW : newness=XFER_UPGRADE;  break;
        /* TODO: for PRUNEDNEW we should merge prios; might be XFER_DOWNGRADE... */
      }
    }

    if (newness!=-1)
    {
      const TYPE_DESC& desc = context.typeDefs()[OBJ_TYPE(theObjTab[i].hdr)];
      DDD_OBJ obj   = HDR2OBJ(theObjTab[i].hdr, &desc);

      assert(theObjTab[i].is_new!=OTHERMSG);

      /* call application handler for object consistency */
      if (desc.handlerOBJMKCONS)
      {
        desc.handlerOBJMKCONS(context, obj, newness);
      }
    }
  }

  /*STAT_INCTIMER4(23);*/
}




/****************************************************************************/


/*
        unpack table of TEOldCpl-items.

   this function is called for each incoming message. for each
        incoming object which hasn't been here before, a set of old
        couplings is added as an estimate until the second xfer-
        communication gives more details.

        for OTHERMSG-objects there is always another object copy
        with TOTALNEW-flag. only the one with TOTALNEW-flag submits
        the set of TEOldCpl, the other sets will be equal (for consistent
        datasets before the Xfer) and are therefore redundant.
 */
static void UnpackOldCplTab (
  DDD::DDDContext& context,
  TEOldCpl *tabOC, int nOC,
  OBJTAB_ENTRY *tabO, int nO)
{
  int iO, iOC;

  iO = iOC = 0;
  while (iOC<nOC && iO<nO)
  {
    /* skip ObjTab-items until a TOTALNEW is found */
    while (iO<nO && tabO[iO].is_new!=TOTALNEW)
      iO++;

    if (iO<nO)
    {
      /* look for TEOldCpl-items with same gid.
         note: this relies on previous sorting via
         sort_XIOldCpl on sender side. */
      while (iOC<nOC && tabOC[iOC].gid<OBJ_GID(tabO[iO].hdr))
        iOC++;

      /* found some TEOldCpl-items with same gid */
      /* add couplings now */
      while (iOC<nOC && tabOC[iOC].gid==OBJ_GID(tabO[iO].hdr))
      {
        AddCoupling(context, tabO[iO].hdr,tabOC[iOC].proc,tabOC[iOC].prio);
        iOC++;
      }

      iO++;
    }
  }
}



/*
        compress table of TENewCpl-items.

        forall sets of NewCpl-items with same gid and same dest,
        compress set according to PriorityMerge(). only the winning
        priority has to survive.

        at first, the table is sorted according to (gid/dest), which
        will construct the NewCpl-sets. afterwards, for each set the winner
        is determined and stored into table. the tablesize will be equal or
        smaller at the end of this function.
 */
static int CompressNewCpl(DDD::DDDContext& context, TENewCpl *tabNC, int nNC)
{
  int nNCnew;
  int iNC;

  std::sort(tabNC, tabNC + nNC, sort_TENewCpl);

  nNCnew = iNC = 0;
  while (iNC<nNC)
  {
    /* TENewCpl.type component is needed here (for merging priorities)! */
    TYPE_DESC *desc  = &context.typeDefs()[NewCpl_GetType(tabNC[iNC])];
    DDD_PRIO newprio;

    newprio = NewCpl_GetPrio(tabNC[iNC]);
    while (iNC<nNC-1 && NewCpl_GetGid(tabNC[iNC+1])==NewCpl_GetGid(tabNC[iNC]) &&
           NewCpl_GetDest(tabNC[iNC+1])==NewCpl_GetDest(tabNC[iNC]))
    {
      PriorityMerge(desc, newprio, NewCpl_GetPrio(tabNC[iNC+1]), &newprio);
      iNC++;
    }

    if (iNC<nNC)
    {
      NewCpl_SetGid(tabNC[nNCnew],  NewCpl_GetGid(tabNC[iNC]));
      NewCpl_SetDest(tabNC[nNCnew], NewCpl_GetDest(tabNC[iNC]));
      NewCpl_SetPrio(tabNC[nNCnew], newprio);
      NewCpl_SetType(tabNC[nNCnew], NewCpl_GetType(tabNC[iNC]));
      nNCnew++;

      iNC++;
    }
  }

  return(nNCnew);
}


/****************************************************************************/



/*
        main unpack procedure.
 */

void XferUnpack (DDD::DDDContext& context, LC_MSGHANDLE *theMsgs, int nRecvMsgs,
                 const DDD_HDR *localCplObjs, int nLocalCplObjs,
                 std::vector<XISetPrio*>& theSP,
                 XIDelObj **arrayDO, int nDO,
                 const std::vector<XICopyObj*>& arrayCO,
                 XICopyObj **arrayNewOwners, int nNewOwners)
{
  auto& ctx = context.xferContext();

  TENewCpl     *allNewCpl;
  OBJTAB_ENTRY **unionObjTab;
  int lenObjTab, lenSymTab, nNewCpl;
  int i, pos1, pos2, len;
  XISetPrio** arraySP = theSP.data();
  const int nSP = theSP.size();



  lenObjTab=lenSymTab=nNewCpl=0;

  for(i=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];
    lenObjTab += (int)LC_GetTableLen(xm, ctx.objtab_id);
    lenSymTab += (int)LC_GetTableLen(xm, ctx.symtab_id);
    nNewCpl += (int)LC_GetTableLen(xm, ctx.newcpl_id);
  }

#       if DebugUnpack<=4
  Dune::dverb << "SUM OF"
              << " OBJ=" << std::setw(3) << lenObjTab
              << " SYM=" << std::setw(3) << lenSymTab
              << " NEW=" << std::setw(3) << nNewCpl
              << " FROM " << std::setw(2) << nRecvMsgs << " MSGS\n";
#       endif

  /*STAT_RESET3;*/

  if (nNewCpl>0)
  {
    allNewCpl = (TENewCpl *) OO_Allocate (sizeof(TENewCpl)*nNewCpl);
    if (allNewCpl == nullptr)
      throw std::bad_alloc();
  } else {
    allNewCpl = NULL;
  }


  if (lenObjTab>0)
  {
    unionObjTab = (OBJTAB_ENTRY **) OO_Allocate (sizeof(OBJTAB_ENTRY *)*lenObjTab);
    if (unionObjTab == nullptr)
      throw std::bad_alloc();
  } else {
    unionObjTab = NULL;
  }


  /*STAT_TIMER3(25); STAT_RESET3;*/


  /* create union tables: allNewCpl, unionObjTab */
  for(i=0, pos1=pos2=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];
    char *theObjects = (char *) LC_GetPtr(xm, ctx.objmem_id);

    len = (int) LC_GetTableLen(xm, ctx.newcpl_id);
    if (len>0)
    {
      memcpy(allNewCpl+pos1, LC_GetPtr(xm,ctx.newcpl_id),
             sizeof(TENewCpl)*len);
      pos1 += len;
    }

    len = (int) LC_GetTableLen(xm, ctx.objtab_id);
    if (len>0)
    {
      OBJTAB_ENTRY *msg_ot = (OBJTAB_ENTRY *)
                             LC_GetPtr(xm,ctx.objtab_id);
      OBJTAB_ENTRY **all_ot = unionObjTab+pos2;
      int oti;
      for(oti=0; oti<len; oti++, all_ot++, msg_ot++)
      {
        *all_ot = msg_ot;

        /* enter pointer to HDR-copy inside message, temporarily */
        msg_ot->hdr = OTE_HDR(theObjects,msg_ot);
      }

      pos2 += len;
    }
  }

  if (nNewCpl>0)
  {
    nNewCpl = CompressNewCpl(context, allNewCpl, nNewCpl);
  }

  if (lenObjTab>0)
  {
    std::sort(unionObjTab, unionObjTab + lenObjTab, sort_ObjTabPtrs);
  }


#       if DebugUnpack<=2
  for(i=0; i<nNewCpl; i++)
  {
    Dune::dvverb << " TAB allNewCpl " << NewCpl_GetGid(allNewCpl[i])
                 << " on " << std::setw(4) NewCpl_GetDest(allNewCpl[i])
                 << "/" << NewCpl_GetPrio(allNewCpl[i]) << "\n";
  }
#       endif



  /* accept all received objects */
  if (nRecvMsgs>0)
  {
    AcceptReceivedObjects(
      context,
      theMsgs, nRecvMsgs,
      unionObjTab, lenObjTab,
      localCplObjs, nLocalCplObjs
      );
  }


  /*
          TODO: the following loops can be implemented more
          efficiently. in each loop, there is another loop
          across all objects inside the message. for each object,
          the TypeDesc is computed. the typedesc pointers should
          be computed once and stored somewhere. kb 970115
   */

  /* insert local references into symtabs */
  for(i=0; i<nRecvMsgs; i++)
    LocalizeSymTab(context, theMsgs[i], unionObjTab, lenObjTab,
                   localCplObjs, nLocalCplObjs);


  /*
          TODO. perhaps the following loops across all messages
          should be split up even further. (i.e., XFER_NEW,
          then XFER_UPGRADE, then XFER_REJECT).
   */


  /* unpack all messages and update local topology */
  for(i=0; i<nRecvMsgs; i++) LocalizeObjects(context, theMsgs[i], true);
  for(i=0; i<nRecvMsgs; i++) LocalizeObjects(context, theMsgs[i], false);

  /*
          at this point all new objects are established,
          their references point correctly to the neighbour objects.
          note: the references from neighbours to the new objects
          are not actualized yet! this has to be done via the
          application handler OBJMKCONS.
   */

  /* KB 941109
          the order of the next steps is crucial:
          1. update objects via handler
          2. add additional data items
          3. call set-prio handlers
          4. update consistency
   */

  /* for NOTNEW,PARTNEW,PRUNEDNEW objects */
  for(i=0; i<nRecvMsgs; i++)
    CallSetPriorityHandler(context, theMsgs[i]);

  /* for TOTALNEW objects */
  for(i=0; i<nRecvMsgs; i++)
    CallUpdateHandler(context, theMsgs[i]);

  /* for all incoming objects */
  for(i=0; i<nRecvMsgs; i++) UnpackAddData(context, theMsgs[i], true);
  for(i=0; i<nRecvMsgs; i++) UnpackAddData(context, theMsgs[i], false);

  /* for PARTNEW and TOTALNEW objects */
  for(i=0; i<nRecvMsgs; i++) CallObjMkConsHandler(context, theMsgs[i], true);
  for(i=0; i<nRecvMsgs; i++) CallObjMkConsHandler(context, theMsgs[i], false);



  /*
   #	if DebugXfer>1
          if (DDD_GetOption(context, OPT_DEBUG_XFERMESGS)==OPT_ON)
   #	endif
          {
                  for(i=0; i<nRecvMsgs; i++)
                          XferDisplayMsg(context, "OR", theMsgs[i]);
          }
   */



  /* unpack all OldCpl-tabs */
  for(i=0; i<nRecvMsgs; i++)
  {
    LC_MSGHANDLE xm = theMsgs[i];
    UnpackOldCplTab(
      context,
      (TEOldCpl *)     LC_GetPtr(xm,ctx.oldcpl_id),
      (int)            LC_GetTableLen(xm, ctx.oldcpl_id),
      (OBJTAB_ENTRY *) LC_GetPtr(xm, ctx.objtab_id),
      (int)            LC_GetTableLen(xm, ctx.objtab_id) );
  }




  /* update couplings according to global cpl tab */
  UpdateCouplings(context,
                  allNewCpl, nNewCpl,
                  unionObjTab, lenObjTab,
                  localCplObjs, nLocalCplObjs,
                  arrayDO, nDO,
                  arrayNewOwners, nNewOwners
                  );

  /* create new XI???Cpl-infos depending on allNewCpls for existing
     objects */
  PropagateCplInfos(context, arraySP, nSP, arrayDO, nDO, allNewCpl, nNewCpl);


  /* create some more XIModCpl-items due to incoming objects */
  PropagateIncomings(context, arrayNewOwners, nNewOwners, unionObjTab, lenObjTab);


  /* free temporary memory */
  if (allNewCpl!=NULL)
    OO_Free (allNewCpl /*,0*/);
  if (unionObjTab!=NULL)
    OO_Free (unionObjTab /*,0*/);
}




END_UGDIM_NAMESPACE
