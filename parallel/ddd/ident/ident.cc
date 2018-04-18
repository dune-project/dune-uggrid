// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ident.c                                                       */
/*                                                                          */
/* Purpose:   object identification for ddd module                          */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/30 kb  begin                                            */
/*            94/04/25 kb  major revision, all Identfy-functions impl.      */
/*            96/02/08 kb  fixed bug in vchannel handling (due to T3D)      */
/*            96/11/25 kb  added indirect identification                    */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

#define DebugIdent 10  /* 10 is off */


/****************************************************************************/


/*
        in debuglevel DebugIdentCons, addititional data is sent with
        the identify-messages, in order to check the consistency of
        the identification tupels.

        this is for configuring the debug actions only.
 */
#define DebugIdentCons  8



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
#include <iostream>
#include <iterator>
#include <tuple>

#include <dune/common/exceptions.hh>
#include <dune/common/stdstreams.hh>

#include <dune/uggrid/parallel/ddd/dddcontext.hh>

#include "dddi.h"

#include "basic/notify.h"


#include "basic/oopp.h"    /* for object-orientated style via preprocessor */

USING_UG_NAMESPACE

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


/* types of IDENTINFO items, ID_OBJECT must be the smallest value!! */
#define ID_OBJECT   1
#define ID_NUMBER   2
#define ID_STRING   3


#define TUPEL_LEN(t)    ((int)((t)&0x3f))


/* overall mode of identification */
enum IdentMode {
  IMODE_IDLE = 0,          /* waiting for next DDD_IdentifyBegin() */
  IMODE_CMDS,              /* after DDD_IdentifyBegin(), before DDD_IdentifyEnd() */
  IMODE_BUSY               /* during DDD_IdentifyEnd() */
};



/***/

/* some macros for customizing oopp */
#define _NEWPARAMS
#define _NEWPARAMS_OR_VOID    void

#define __INDENT(n)   { int i; for(i=0; i<n; i++) fputs("   ",fp);}
#define _PRINTPARAMS  , int indent, FILE *fp
#define _PRINTPARAMS_DEFAULT  ,0,stdout
#define _INDENT       __INDENT(indent)
#define _INDENT1      __INDENT(indent+1)
#define _INDENT2      __INDENT(indent+2)
#define _PRINTNEXT    , indent+1, fp
#define _PRINTSAME    , indent, fp


/* map memory allocation calls */
#define OO_Allocate  std::malloc
#define OO_Free      std::free


/* extra prefix for all xfer-related data structures and/or typedefs */
/*#define ClassPrefix*/



/*
    NOTE: all container-classes from ooppcc.h are implemented in this
          source file by setting the following define.
 */
#define ContainerImplementation
#define _CHECKALLOC(ptr)   assert(ptr!=NULL)


/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/* IDENTIFIER:                                                              */
/****************************************************************************/

union IDENTIFIER {
  int number;
  char       *string;
  DDD_GID object;
};



/****************************************************************************/
/* MSGITEM:                                                                 */
/****************************************************************************/

struct MSGITEM {
  DDD_GID gid;
  DDD_PRIO prio;

#       if DebugIdent<=DebugIdentCons
  unsigned long tupel;         /* send tupel ID for checking consistency */
#       endif

};



/****************************************************************************/
/* IDENTINFO:                                                               */
/****************************************************************************/

struct ID_TUPEL;

struct IDENTINFO {
  int typeId;
  int entry;
  IDENTIFIER id;


  MSGITEM msg;                     /* this item is sent to other procs */

  DDD_HDR hdr;

  ID_TUPEL  *tupel;
};



/****************************************************************************/
/* ID_TUPEL:                                                                */
/****************************************************************************/

struct ID_REFDBY
{
  IDENTINFO  *by;
  ID_REFDBY  *next;
};


struct ID_TUPEL {
  unsigned long tId;
  IDENTINFO     **infos;

  int nObjIds;                    /* number of entries with typeID==ID_OBJECT */

  int loi;                        /* level of indirection */
  ID_REFDBY     *refd;            /* list of referencing IdEntries */
};


/****************************************************************************/
/* IdEntry:                                                                 */
/****************************************************************************/


struct IdEntry {
  IDENTINFO msg;
};


/* define container class */
#define SegmListOf   IdEntry
#define SegmSize     128
#include "basic/ooppcc.h"




/****************************************************************************/
/* ID_PLIST:                                                                */
/****************************************************************************/

struct ID_PLIST {
  DDD_PROC proc;
  int nEntries;
  int nIdentObjs;

  ID_PLIST *next;
  IdEntrySegmList  *entries;

  IDENTINFO   **local_ids;
  ID_TUPEL    *indexmap;                  /* index-mapping of local_ids array */

  MSGITEM     *msgin, *msgout;
  msgid idin, idout;
};


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



static ID_PLIST   *thePLists;
static int cntIdents, nPLists;

static enum IdentMode identMode;



/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*
        management functions for IdentMode.

        these functions control the mode the ident-module is
        currently in. this is used for error detection, but
        also for correct detection of coupling inconsistencies
        and recovery.
 */

static const char *IdentModeName (enum IdentMode mode)
{
  switch(mode)
  {
  case IMODE_IDLE : return "idle-mode";
  case IMODE_CMDS : return "commands-mode";
  case IMODE_BUSY : return "busy-mode";
  }
  return "unknown-mode";
}


static void IdentSetMode (enum IdentMode mode)
{
  identMode = mode;

#       if DebugIdent<=8
  Dune::dinfo << "IdentMode=" << IdentModeName(identMode) << "\n";
#       endif
}


static enum IdentMode IdentSuccMode (enum IdentMode mode)
{
  switch(mode)
  {
  case IMODE_IDLE : return IMODE_CMDS;
  case IMODE_CMDS : return IMODE_BUSY;
  case IMODE_BUSY : return IMODE_IDLE;
  }
  return IMODE_IDLE;
}



/*
   static int IdentMode (void)
   {
        return identMode;
   }
 */


static int IdentActive (void)
{
  return identMode!=IMODE_IDLE;
}


static int IdentStepMode (enum IdentMode old)
{
  if (identMode!=old)
    DUNE_THROW(Dune::Exception,
               "wrong Ident-mode (currently in " << IdentModeName(identMode)
               << ", expected " << IdentModeName(old) << ")");

  IdentSetMode(IdentSuccMode(identMode));
  return true;
}


/****************************************************************************/


static void PrintPList (ID_PLIST *plist)
{
  using std::setw;
  std::ostream& out = std::cout;

  out << "PList proc=" << setw(4) << plist->proc
      << " entries=" << setw(5) << plist->nEntries << "\n";
}



/****************************************************************************/


static int compareId (const IDENTINFO *el1, const IDENTINFO *el2)
{
  int cmp;

  /* first compare id type (NUMBER, STRING or OBJECT) */
  if (el1->typeId < el2->typeId) return(-1);
  if (el1->typeId > el2->typeId) return(1);


  /* same typeIds, compare identification then */
  switch (el1->typeId) {
  case ID_NUMBER :
    if (el1->id.number < el2->id.number) return(-1);
    if (el1->id.number > el2->id.number) return(1);
    break;

  case ID_STRING :
    cmp = strcmp(el1->id.string, el2->id.string);
    if (cmp!=0) return(cmp);
    break;

  case ID_OBJECT :
    if (el1->id.object < el2->id.object) return(-1);
    if (el1->id.object > el2->id.object) return(1);
    break;
  }

  return(0);
}



/****************************************************************************/

/*
        two functions for sorting IdentifyXXX-requests into tupels:

        sort_intoTupelsLists keeps order of IdentifyXXX-issueing by
           application program, i.e., the ordering is relevant

        sort_intoTupelsSets reorders the IdentifyXXX-items inside each
           tupel itself;
           at this level the ordering is done only by typeId, where
           ID_OBJECT comes first. lateron the IdentifyObject-items will
           be sorted according to their gid (for objects with loi==0)
           or the index of the loi-1 object (for objects with loi>0)
 */


static bool sort_intoTupelsLists(const IDENTINFO* a, const IDENTINFO* b)
{
  /* sort according to (old) global ids */
  /* if equal, keep ordering of input  */
  return std::tie(a->msg.gid, a->entry) < std::tie(b->msg.gid, b->entry);
}

static bool sort_intoTupelsSets(const IDENTINFO* a, const IDENTINFO* b)
{
  /* sort according to (old) global ids */
  /* if equal, sort according to identificator itself */
  if (a->msg.gid < b->msg.gid)
    return true;
  else if (a->msg.gid > b->msg.gid)
    return false;
  else
    return compareId(a, b) < 0;
}




/****************************************************************************/


static int sort_tupelOrder (const void *e1, const void *e2)
{
  ID_TUPEL *el1, *el2;
  int cmp, i, nIds;
  DDD_HDR el1hdr, el2hdr;

  el1 = (ID_TUPEL *) e1;
  el2 = (ID_TUPEL *) e2;


  /* sort according to tupel id */
  if (el1->tId < el2->tId) return(-1);
  if (el1->tId > el2->tId) return(1);

  /* ids are equal, sort according tupel value */

  /* recode tupel length from lowest 6 bits */
  nIds = TUPEL_LEN(el1->tId);


  /* compare until one tupel entry differs */
  for(i=0; i<nIds; i++) {
    if ((cmp=compareId(el1->infos[i], el2->infos[i])) != 0)
      return(cmp);
  }


  /* if tupels are equal by all means up to now, we
     sort according to DDD_TYPE of local object.
     hence, we can identify two pairs of local objects with
     the same tupel.
     this has to be ommitted if objects with different
     types should be identifiable. KB 960814
   */
  el1hdr = el1->infos[0]->hdr;
  el2hdr = el2->infos[0]->hdr;
  if (OBJ_TYPE(el1hdr) < OBJ_TYPE(el2hdr)) return(-1);
  if (OBJ_TYPE(el1hdr) > OBJ_TYPE(el2hdr)) return(1);


  if (el1hdr!=el2hdr) {
    /*
       for(i=0; i<nIds; i++) {
            printf("%4d: tupel[%d]  %08x/%d  %08x/%d   (id/loi)\n",
                    me, i,
                    el1->infos[i]->id.object, el1->loi,
                    el2->infos[i]->id.object, el2->loi);
       }
     */

    DUNE_THROW(Dune::Exception,
               "same identification tupel for objects "
               << OBJ_GID(el1hdr) << " and " << OBJ_GID(el2hdr));
  }

  return(0);
}


/****************************************************************************/

static void SetLOI (IDENTINFO *ii, int loi)
{
  ID_REFDBY *rby;
  ID_TUPEL  *tupel = ii->tupel;

  /*
     printf("%4d: %08x SetLOI(%d, %d)\n", me, ii->msg.gid, loi, ii->loi);
   */

  /* set loi to maximum of current and new value */
  tupel->loi = MAX(loi, tupel->loi);

  /* primitive cycle detection */
  if (tupel->loi > 64)
    DUNE_THROW(Dune::Exception,
               "IdentifyObject-cycle, objects "
               << ii->msg.gid << " and " << ii->id.object);


  for(rby=tupel->refd; rby!=NULL; rby=rby->next)
  {
    SetLOI(rby->by, loi+1);

    /* TODO detection of cycles */
  }
}



static void ResolveDependencies (
  ID_TUPEL  *tupels, int nTupels,
  IDENTINFO **id, int nIds, int nIdentObjs)
{
  int i, j;

  if (nIdentObjs==0)
    return;

  std::vector<IDENTINFO*> refd;
  refd.reserve(nIdentObjs);

  /* build array of pointers to objects being used for identification */
  std::copy_if(
    id, id + nIds,
    std::back_inserter(refd),
    [](const IDENTINFO* ii) { return ii->typeId == ID_OBJECT; }
    );
  assert(refd.size() == nIdentObjs);

  /* sort it according to GID of referenced objects */
  std::sort(
    refd.begin(), refd.end(),
    [](const IDENTINFO* a, const IDENTINFO* b) {
      return a->id.object < b->id.object;
    });

  /*
     for(j=0; j<nIdentObjs; j++)
          printf("%4d: DepObj %08x  (from %08x)\n",
                  me, refd[j]->id.object, OBJ_GID(refd[j]->hdr));
   */


  for(i=0, j=0; i<nTupels; i++)
  {
    while (j<nIdentObjs &&
           refd[j]->id.object < tupels[i].infos[0]->msg.gid)
      j++;

    while (j<nIdentObjs &&
           refd[j]->id.object == tupels[i].infos[0]->msg.gid)
    {
      ID_REFDBY *rby = new ID_REFDBY;

      /* remember that idp[i] is referenced by refd[j] */
      rby->by        = refd[j];
      rby->next      = tupels[i].refd;
      tupels[i].refd = rby;

      j++;
    }
  }


  for(i=0; i<nTupels; i++)
  {
    ID_REFDBY *rby;

    for(rby=tupels[i].refd; rby!=NULL; rby=rby->next)
    {
      /* if loi>0, this subtree has been loi-ed before */
      if (tupels[i].loi==0)
        SetLOI(rby->by, tupels[i].loi+1);
    }
  }


#       if DebugIdent<=2
  /* display */
  for(i=0; i<nTupels; i++)
  {
    ID_REFDBY *rby;

    printf("%4d: %08x has loi %d\n",
           me, tupels[i].infos[0]->msg.gid, tupels[i].loi);

    for(rby=tupels[i].refd; rby!=NULL; rby=rby->next)
    {
      printf("%4d: %08x referenced by %08x\n",
             me, tupels[i].infos[0]->msg.gid, rby->by->msg.gid);
    }
  }
#       endif
}



static void CleanupLOI (ID_TUPEL *tupels, int nTupels)
{
  int i;

  for(i=0; i<nTupels; i++)
  {
    ID_REFDBY *rby, *next=0;

    for(rby=tupels[i].refd; rby!=NULL; rby=next)
    {
      next = rby->next;

      /* TODO use freelists */
      delete rby;
    }
  }
}


/****************************************************************************/


/*
        tupel-id doesn't contain information about the data in
        the tupel, it does only contain information about the
        structure of a tupel!
 */
static void TupelInit (ID_TUPEL *tupel, IDENTINFO **id, int nIds)
{
  int i, nObjIds;
  unsigned long tId;

  /* init tupel auxiliary data */
  tupel->loi  = 0;
  tupel->refd = NULL;


  /* compute tupel id */
  tId = 0;
  nObjIds = 0;
  for(i=0; i<nIds; i++)
  {
    tId = (tId<<2) | id[i]->typeId;

    /* count entries with ID_OBJECT */
    if (id[i]->typeId==ID_OBJECT)
      nObjIds++;
  }


  /* code length of tupel into lowest 6 bits */
  tId = (tId<<6) | nIds;

  /* printf("%4d: compute tupel id = %08x\n", me, tId); */


  /* insert tupel id, number of ID_OBJECT-entries, and link to
     array of pointers to the tupel's IDENTINFO structs */
  tupel->tId     = tId;
  tupel->nObjIds = nObjIds;
  tupel->infos   = id;


  /* set link from IDENTINFOs to tupel */
  for(i=0; i<nIds; i++)
  {
    id[i]->tupel = tupel;
  }
}



static int IdentifySort (const DDD::DDDContext& context,
                         IDENTINFO **id, int nIds,
                         int nIdentObjs, MSGITEM *items_out, ID_TUPEL **indexmap_out,
                         DDD_PROC dest
                         )
{
  int i, j, last, nTupels;
  int keep_order_inside_tupel;


  /* sort to recognize identification tupels */
  /* in case of IDMODE_LISTS, the original ordering
     inside each tupel is kept. for IDMODE_SETS, each tupel
     is sorted according to the identificators themselves. */
  STAT_RESET3;
  switch (DDD_GetOption(context, OPT_IDENTIFY_MODE))
  {
  case IDMODE_LISTS :
    std::sort(id, id + nIds, sort_intoTupelsLists);
    keep_order_inside_tupel = true;
    break;

  case IDMODE_SETS :
    std::sort(id, id + nIds, sort_intoTupelsSets);
    keep_order_inside_tupel = false;
    break;

  default :
    DUNE_THROW(Dune::Exception, "unknown OPT_IDENTIFY_MODE");
  }
  STAT_INCTIMER3(T_QSORT_TUPEL);


  /* compute number of tupels and allocate tupel array */
  for(i=0, last=0, nTupels=1; i<nIds; i++)
  {
    if (id[i]->msg.gid > id[last]->msg.gid)
    {
      nTupels++;
      last=i;
    }
  }
  ID_TUPEL* tupels = new ID_TUPEL[nTupels];

  /* init tupels (e.g., compute tupel ids) */
  for(i=0, last=0, j=0; i<nIds; i++)
  {
    if (id[i]->msg.gid > id[last]->msg.gid)
    {
      TupelInit(&(tupels[j]), &(id[last]), i-last);
      j++;
      last=i;
    }
  }
  TupelInit(&(tupels[j]), &(id[last]), nIds-last);


  /*
          now, 'tupels' is an array of identification tupels,
          sorted according the gid of the object the tupel has
          been specified for.

          i.e., in a more abstract way tupels is a list of object
          gids which will be identified.
   */

  /* resolve dependencies caused by IdentifyObject,
     and set level-of-indirection accordingly */
  STAT_RESET3;
  ResolveDependencies(tupels, nTupels, id, nIds, nIdentObjs);
  STAT_INCTIMER3(T_RESOLVE_DEP);


  /*
          the setting for loi is used for the next sorting procedure,
          first level of indirection comes first.
   */

  /* sort array for loi */
  STAT_RESET3;
  std::sort(
    tupels, tupels + nTupels,
    [](const ID_TUPEL& a, const ID_TUPEL& b) { return a.loi < b.loi; }
    );
  STAT_INCTIMER3(T_QSORT_LOI);


  STAT_RESET3;
  i=0; j=0;
  do {
    while (j<nTupels && tupels[i].loi==tupels[j].loi)
    {
      /* reorder because of changes in id.object */
      if (! keep_order_inside_tupel)
      {
        std::sort(
          tupels[j].infos, tupels[j].infos + tupels[j].nObjIds,
          sort_intoTupelsSets
          );
      }
      j++;
    }

    /* sort sub-array for tupelId, tupelValue */
    if (j-i > 1)
      qsort(tupels+i, j-i, sizeof(ID_TUPEL), sort_tupelOrder);

    /* inherit index to tupels referencing this one */
    while (i<j)
    {
      ID_REFDBY *rby;

      for(rby=tupels[i].refd; rby!=NULL; rby=rby->next)
      {
        /* dont use gid of referenced object (because it
           will be known only after identification), but its
           position in the identification table instead! */

        /*
           printf("%4d: insertRef dest=%d loi=%d i=%d, %08x <- %08x\n",
           me, dest, tupels[i].loi, i, OBJ_GID(tupels[i].infos[0]->hdr),
           OBJ_GID(rby->by->hdr));
         */


        rby->by->id.object = i;

        /* if the ordering is not significant, we must reorder
           the tupel after this operation. (i.e., for IDMODE_SETS).
         */
      }

      i++;
    }
    /* now i==j */

  } while (i<nTupels);
  STAT_INCTIMER3(T_BUILD_GRAPH);


  /* construct array which will be sent actually */
  STAT_RESET3;
  for(j=0; j<nTupels; j++)
  {
    /*
       int k;
     */
#               if DebugIdent<=1
    printf("%4d: Ident dest=%d msg_idx[ %08x ] = %5d, loi=%d\n",
           me, dest, tupels[j].infos[0]->msg.gid, j, tupels[j].loi);
#               endif

    /*
       for(k=0;k<tupels[j].nObjIds;k++)
       {
            printf("%4d:               msg_idx %d %08x\n", me,
                    k, tupels[j].infos[k]->id.object);
       }
     */



    items_out[j] = tupels[j].infos[0]->msg;

#               if DebugIdent<=DebugIdentCons
    /* send additional data for cons-checking */
    items_out[j].tupel = tupels[j].tId;
#               endif
  }
  STAT_INCTIMER3(T_CONSTRUCT_ARRAY);


  CleanupLOI(tupels, nTupels);

  /* return indexmap table, in order to keep ordering of tupels */
  /* note: this array has to be freed in the calling function! */
  *indexmap_out = tupels;

  return(nTupels);
}



static int InitComm(DDD::DDDContext& context, int nPartners)
{
  ID_PLIST  *plist;
  int i, err;
  DDD_PROC  *partners = DDD_ProcArray(context);

  /* fill partner processor numbers into array */
  for(plist=thePLists, i=0; i<nPartners; i++, plist=plist->next)
    partners[i] = plist->proc;

  if (! IS_OK(DDD_GetChannels(context, nPartners)))
  {
    return(false);
  }


  /* initiate asynchronous receives and sends */
  for(plist=thePLists; plist!=NULL; plist=plist->next)
  {
    long *len_adr;

    plist->idin = RecvASync(context.ppifContext(), VCHAN_TO(context, plist->proc),
                            ((char *)plist->msgin) - sizeof(long),
                            sizeof(MSGITEM)*plist->nEntries + sizeof(long), &err);

    /* store number of entries at beginning of message */
    len_adr = (long *) (((char *)plist->msgout) - sizeof(long));
    *len_adr = plist->nEntries;
    plist->idout = SendASync(context.ppifContext(), VCHAN_TO(context, plist->proc),
                             ((char *)plist->msgout) - sizeof(long),
                             sizeof(MSGITEM)*plist->nEntries + sizeof(long), &err);
  }

  return(true);
}


/****************************************************************************/

/*
        this routine checks whether number of idents per proc is indeed
        pairwise consistent.
 */

static void idcons_CheckPairs(DDD::DDDContext& context)
{
  NOTIFY_DESC *msgs = DDD_NotifyBegin(context, nPLists);
  ID_PLIST        *plist;
  int i, j, nRecvs, err=false;

  for(i=0, plist=thePLists; plist!=NULL; plist=plist->next, i++)
  {
    msgs[i].proc = plist->proc;
    msgs[i].size = plist->nEntries;
  }

  /* communicate */
  nRecvs = DDD_Notify(context);
  if (nRecvs==ERROR)
    DUNE_THROW(Dune::Exception, "Notify failed in Ident-ConsCheck");


  /* perform checking */
  for(plist=thePLists; plist!=NULL; plist=plist->next)
  {
    for(j=0; j<nRecvs; j++)
    {
      if (msgs[j].proc==plist->proc)
        break;
    }

    if (j==nRecvs)
    {
      Dune::dgrave << "Identify: no Ident-calls from proc " << plist->proc
                   << ", expected " << plist->nEntries << "\n";
      err=true;
    }
    else if (msgs[j].size!=plist->nEntries)
    {
      Dune::dgrave << "Identify: " << msgs[j].size << " Ident-calls from proc "
                   << plist->proc << ", expected " << plist->nEntries << "\n";
      err=true;
    }
  }

  DDD_NotifyEnd(context);

  if (err)
  {
    DUNE_THROW(Dune::Exception, "found errors in IdentifyEnd()");
  }
  else
  {
    Dune::dwarn << "Ident-ConsCheck level 0: ok\n";
  }
}



/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* Function:  DDD_IdentifyEnd                                               */
/*                                                                          */
/****************************************************************************/

/**
        End of identication phase.
        This function starts the object identification process. After a call to
        this function (on all processors) all {\bf Identify}-commands since the
        last call to \funk{IdentifyBegin} are executed. This involves a set
        of local communications between the processors.
 */

DDD_RET DDD_IdentifyEnd(DDD::DDDContext& context)
{
  ID_PLIST        *plist, *pnext=NULL;
  int cnt, j;

  /* REMARK: dont use the id->msg.msg.prio fields until they
          are explicitly set at line L1!                         */

  STAT_SET_MODULE(DDD_MODULE_IDENT);
  STAT_ZEROALL;

#       if DebugIdent<=9
  printf("%4d: DDD_IdentifyEnd.\n", me);
  fflush(stdout);
#       endif

  /* step mode and check whether call to IdentifyEnd is valid */
  if (!IdentStepMode(IMODE_CMDS))
    DUNE_THROW(Dune::Exception, "DDD_IdentifyEnd() aborted");


#   if DebugIdent<=9
  idcons_CheckPairs(context);
#       endif


  STAT_RESET1;

  /* for each id_plist entry */
  for(plist=thePLists, cnt=0; plist!=NULL; plist=plist->next, cnt++)
  {
    /* allocate message buffers */
    /* use one alloc for three buffers */
    plist->local_ids = (IDENTINFO **) std::malloc(
      sizeof(IDENTINFO *)*plist->nEntries +                    /* for local id-infos  */
      sizeof(long) +                                           /* len of incoming msg */
      sizeof(MSGITEM)    *plist->nEntries +                    /* for incoming msg    */
      sizeof(long) +                                           /* len of outgoing msg */
      sizeof(MSGITEM)    *plist->nEntries                      /* for outgoing msg    */
      );

    if (plist->local_ids==NULL)
      throw std::bad_alloc();

    plist->msgin  = (MSGITEM *)
                    (((char *)&plist->local_ids[plist->nEntries]) + sizeof(long));
    plist->msgout = (MSGITEM *)
                    (((char *)&plist->msgin[plist->nEntries]) + sizeof(long));


    /* construct pointer array to IDENTINFO structs */
    /* AND: fill in current priority from object's header */
    {
      IdEntrySegm *li;
      int i = 0;

      for(li=plist->entries->first; li!=NULL; li=li->next)
      {
        int entry;
        for(entry=0; entry<li->nItems; entry++)
        {
          IdEntry *id = &(li->data[entry]);

          plist->local_ids[i] = &(id->msg);
          id->msg.msg.prio = OBJ_PRIO(id->msg.hdr);                               /* L1 */
          i++;
        }
      }
    }


    /* sort outgoing items */
    STAT_RESET2;
    plist->nEntries = IdentifySort(context,
                                   plist->local_ids, plist->nEntries,
                                   plist->nIdentObjs,
                                   plist->msgout,  /* output: msgbuffer outgoing */
                                   &plist->indexmap, /* output: mapping of indices to local_ids array */
                                   plist->proc);
    STAT_INCTIMER2(T_PREPARE_SORT);


#               if DebugIdent<=5
    PrintPList(plist);
#               endif
  }
  STAT_TIMER1(T_PREPARE);
  STAT_SETCOUNT(N_PARTNERS, cnt);

  /* initiate comm-channels and send/receive calls */
  STAT_RESET1;
  if (!InitComm(context, cnt))
    DUNE_THROW(Dune::Exception, "DDD_IdentifyEnd() aborted");


  /*
          each pair of processors now has a plist with one copy on
          each side. the actual OBJ_GID is computed as the minimum
          of the two local object ids on each processor.
   */

#       if DebugIdent<=4
  printf("%4d: DDD_IdentifyEnd. PLists ready.\n", me); fflush(stdout);
#       endif


  /* poll receives */
  for(plist=thePLists, j=0; j<cnt; )
  {
    if (plist->msgin!=NULL)
    {
      int ret, i;

      if ((ret=InfoARecv(context.ppifContext(), VCHAN_TO(context, plist->proc), plist->idin))==1)
      {
        /* process single plist */
        MSGITEM   *msgin  = plist->msgin;
        ID_TUPEL  *msgout = plist->indexmap;

        /* check control data */
        long *len_adr = (long *) (((char *)msgin) - sizeof(long));
        if (*len_adr != plist->nEntries)
          DUNE_THROW(Dune::Exception,
                     "Identify: " << ((int) *len_adr) << " identified objects"
                     << " from proc " << plist->proc << ", expected "
                     << plist->nEntries);

        for(i=0; i<plist->nEntries; i++, msgin++, msgout++)
        {
#                                       if DebugIdent<=1
          printf("%4d: identifying %08x with %08x/%d to %08x\n", me,
                 OBJ_GID(msgout->infos[0]->hdr), msgin->gid,
                 plist->proc,
                 MIN(OBJ_GID(msgout->infos[0]->hdr), msgin->gid));
#                                       endif

#                                       if DebugIdent<=DebugIdentCons
          if (msgout->tId != msgin->tupel)
          {
            DUNE_THROW(Dune::Exception,
                       "inconsistent tupels, gid "
                       << OBJ_GID(msgout->infos[0]->hdr) << " on " << me
                       << ", gid " << msgin->gid << " on " << plist->proc);
          }
#                                       endif

          /* compute new GID from minimum of both current GIDs */
          OBJ_GID(msgout->infos[0]->hdr) =
            MIN(OBJ_GID(msgout->infos[0]->hdr), msgin->gid);

          /* add a coupling for new object copy */
          AddCoupling(context, msgout->infos[0]->hdr, plist->proc, msgin->prio);
        }

        /* free indexmap (=tupel) array */
        delete[] plist->indexmap;

        /* mark plist as finished */
        plist->msgin=NULL;
        j++;
      }
      else
      {
        if (ret==-1)
          DUNE_THROW(Dune::Exception,
                     "couldn't receive message from " << plist->proc);
      }
    }

    /* next plist, perhaps restart */
    plist=plist->next; if (plist==NULL) plist=thePLists;
  };
  STAT_TIMER1(T_COMM_AND_IDENT);

  /* poll sends */
  for(plist=thePLists; plist!=0; plist=pnext)
  {
    pnext = plist->next;

    /* wait for correct send and free buffer */
    while(InfoASend(context.ppifContext(), VCHAN_TO(context, plist->proc), plist->idout)!=1)
      ;

    /* now, the plist->entries list isn't needed anymore, free */
    IdEntrySegmList_Free(plist->entries);

    std::free(plist->local_ids);
    delete plist;
  };


#       if DebugIdent<=8
  printf("%4d: DDD_IdentifyEnd. Rebuilding interfaces.\n", me);
  fflush(stdout);
#       endif



  /* rebuild interfaces after topological change */
  STAT_RESET1;
  IFAllFromScratch(context);
  STAT_TIMER1(T_BUILD_IF);


#       if DebugIdent<=9
  printf("%4d: DDD_IdentifyEnd. Ready.\n", me); fflush(stdout);
#       endif

  IdentStepMode(IMODE_BUSY);

  return(DDD_RET_OK);
}



/****************************************************************************/


static IdEntry *IdentifyIdEntry (DDD_HDR hdr, DDD_PROC proc, int typeId)
{
  IdEntry     *id;
  ID_PLIST        *plist;

  /* check whether Identify-call is valid */
  if (!IdentActive())
    DUNE_THROW(Dune::Exception, "Missing DDD_IdentifyBegin(), aborted");

  if (proc==me)
    DUNE_THROW(Dune::Exception,
               "cannot identify " << OBJ_GID(hdr) << " with myself");

  if (proc>=procs)
    DUNE_THROW(Dune::Exception,
               "cannot identify " << OBJ_GID(hdr)
               << " with processor " << proc);



  /* search current plist entries */
  for(plist=thePLists; plist!=NULL; plist=plist->next) {
    if (plist->proc==proc)
      break;
  }

  if (plist==NULL)
  {
    /* get new id_plist record */
    plist = new ID_PLIST;

    plist->proc = proc;
    plist->nEntries = 0;
    plist->entries = New_IdEntrySegmList();
    plist->nIdentObjs = 0;
    plist->next = thePLists;
    thePLists = plist;
    nPLists++;
  }


  /* insert into current plist */
  id =  IdEntrySegmList_NewItem(plist->entries);
  id->msg.typeId   = typeId;
  id->msg.hdr      = hdr;
  id->msg.msg.gid  = OBJ_GID(hdr);

  plist->nEntries++;
  if (id->msg.typeId==ID_OBJECT) {
    plist->nIdentObjs++;
  }


  /* NOTE: priority can change between Identify-command and IdentifyEnd!
     therefore, scan priorities at the beginning of IdentifyEnd, and not
     here. KB 970730.
          id->msg.msg.prio = OBJ_PRIO(hdr);
   */

  id->msg.entry = cntIdents++;

  return(id);
}


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_IdentifyNumber                                            */
/*                                                                          */
/****************************************************************************/

/**
        DDD Object identification via integer number.
        After an initial call to \funk{IdentifyBegin}, this function
        identifies two object copies on separate processors. It has to be
        called on both processors with the same identification value.
        The necessary actions (e.g. message transfer) are executed via the
        final call to \funk{IdentifyEnd}; therefore a whole set of
        \funk{Identify}-operations is accumulated.

        After the identification both objects have the same DDD global
        object ID, which is build using the minimum of both local object IDs.

        The identification specified here may be detailed even further by
        additional calls to {\bf Identify}-operations with the same
        local object. This will construct an identification tupel from
        all {\bf Identify}-commands for this local object.

   @param hdr   DDD local object which has to be identified with another object
   @param proc  Owner (processor number) of corresponding local object. This processor has to issue a corresponding call to this {\bf DDD\_Identify}-function.
   @param ident Identification value. This is an arbitrary number to identify two corresponding operations on different processors.
 */

void DDD_IdentifyNumber(DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc, int ident)
{
IdEntry *id;

id = IdentifyIdEntry(hdr, proc, ID_NUMBER);
if (id==NULL)
  throw std::bad_alloc();

id->msg.id.number = ident;

        #if DebugIdent<=2
printf("%4d: IdentifyNumber %08x %02d with %4d num %d\n", me,
       OBJ_GID(hdr), OBJ_TYPE(hdr), proc, id->msg.id.number);
        #endif
}


/****************************************************************************/
/*                                                                          */
/* Function:  DDD_IdentifyString                                            */
/*                                                                          */
/****************************************************************************/

/**
        DDD Object identification via character string.
        After an initial call to \funk{IdentifyBegin}, this function
        identifies two object copies on separate processors. It has to be
        called on both processors with the same identification string.
        The necessary actions (e.g. message transfer) are executed via the
        final call to \funk{IdentifyEnd}; therefore a whole set of
        \funk{Identify}-operations is accumulated.

        After the identification both objects have the same DDD global
        object ID, which is build using the minimum of both local object IDs.

        The identification specified here may be detailed even further by
        additional calls to {\bf Identify}-operations with the same
        local object. This will construct an identification tupel from
        all {\bf Identify}-commands for this local object.

   @param hdr   DDD local object which has to be identified with another object
   @param proc  Owner (processor number) of corresponding local object. This processor has to issue a corresponding call to this {\bf DDD\_Identify}-function.
   @param ident Identification value. This is an arbitrary string to identify two corresponding operations on different processors.
 */

void DDD_IdentifyString(DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc, char *ident)
{
IdEntry *id;

id = IdentifyIdEntry(hdr, proc, ID_STRING);
if (id==NULL)
  throw std::bad_alloc();

id->msg.id.string = ident;

        #if DebugIdent<=2
printf("%4d: IdentifyString %08x %02d with %4d str %s\n", me,
       OBJ_GID(hdr), OBJ_TYPE(hdr), proc, id->msg.id.string);
        #endif
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_IdentifyObject                                            */
/*                                                                          */
/****************************************************************************/

/**
        DDD Object identification via another DDD Object.
        After an initial call to \funk{IdentifyBegin}, this function
        identifies two object copies on separate processors. It has to be
        called on both processors with the same identification object.
        The necessary actions (e.g. message transfer) are executed via the
        final call to \funk{IdentifyEnd}; therefore a whole set of
        \funk{Identify}-operations is accumulated.

        After the identification both objects have the same DDD global
        object ID, which is build using the minimum of both local object IDs.

        The identification object {\em ident} must be either a distributed
        object known to both processors issueing the \funk{IdentifyObject}-command
        or a local object which is not known to these two processors, but which
        will also be identified during the current {\bf Identify}-process.

        The identification specified here may be detailed even further by
        additional calls to {\bf Identify}-operations with the same
        local object. This will construct an identification tupel from
        all {\bf Identify}-commands for this local object.

   @param hdr   DDD local object which has to be identified with another object
   @param proc  Owner (processor number) of corresponding local object. This processor has to issue a corresponding call to this {\bf DDD\_Identify}-function.
   @param ident Identification object. This is an arbitrary global object which is known to both processors involved to identify the two corresponding operations on these processors.
 */

void DDD_IdentifyObject(DDD::DDDContext& context, DDD_HDR hdr, DDD_PROC proc, DDD_HDR ident)
{
IdEntry *id;

id = IdentifyIdEntry(hdr, proc, ID_OBJECT);
if (id==NULL)
  throw std::bad_alloc();

/* use OBJ_GID as estimate for identification value, this estimate
   might be replaced when the corresponding object is identified
   itself. then its index in the identify-message will be used.
   remember identification value in order to replace above estimate,
   if necessary (i.e., remember ptr to ddd-hdr) */
id->msg.id.object = OBJ_GID(ident);

        #if DebugIdent<=2
printf("%4d: IdentifyObject %08x %02d with %4d gid %08x\n", me,
       OBJ_GID(hdr), OBJ_TYPE(hdr), proc, id->msg.id.object);
        #endif
}



/****************************************************************************/
/*                                                                          */
/* Function:  DDD_IdentifyBegin                                             */
/*                                                                          */
/****************************************************************************/

/**
        Begin identification phase.
        A call to this function establishes a global identification operation.
        It should be issued on all processors. After this call an arbitrary
        series of {\bf Identify}-commands may be issued. The global
        identification operation is carried out via a \funk{IdentifyEnd}
        call on each processor.

        All identification commands given for one local object will be collected
        into an {\em identification tupel}. Thus, object identificators can be
        constructed from several simple identification calls. DDD option
   #IDENTIFY_MODE# may be set before the \funk{IdentifyEnd} call
        in order to specify how the order of simple identificators is
        handled for each complex identification tupel:

        \begin{description}
        \item[#IDMODE_LISTS#:]%
        The order of all identification commands for one local object is kept.
        Both processors with corresponding complex identificators must issue
        the identification commands in the same order.
        %
        \item[#IDMODE_SETS#:]%
        The order of all identification commands for one local object is
        not relevant. The DDD identification module sorts the commands
        inside each complex identificator. Both processors with corresponding
        identification tupels may issue the identification commands in any
        order.
        \end{description}
 */

void DDD_IdentifyBegin(DDD::DDDContext& context)
{
  /* step mode and check whether call to IdentifyBegin is valid */
  if (!IdentStepMode(IMODE_IDLE))
    DUNE_THROW(Dune::Exception, "DDD_IdentifyBegin() aborted");

  thePLists    = NULL;
  nPLists      = 0;
  cntIdents    = 0;
}


/****************************************************************************/


void ddd_IdentInit(DDD::DDDContext& context)
{
  IdentSetMode(IMODE_IDLE);
}


void ddd_IdentExit(DDD::DDDContext&)
{}

/****************************************************************************/

END_UGDIM_NAMESPACE
