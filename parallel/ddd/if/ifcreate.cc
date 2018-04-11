// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      ifcreate.c                                                    */
/*                                                                          */
/* Purpose:   routines concerning interfaces between processors             */
/*            part 1: creating and maintaining interfaces                   */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   93/11/30 kb  begin                                            */
/*            94/03/03 kb  complete rewrite                                 */
/*            94/09/12 kb  IFExchange & IFOneway rewrite, two bugs fixed    */
/*            94/09/21 kb  created from if.c                                */
/*            95/01/13 kb  added range functionality                        */
/*            96/01/08 kb  renamed range to attr                            */
/*            96/01/16 kb  added DDD_OBJ shortcut to avoid indirect addr.   */
/*            96/05/07 kb  removed need for global const MAX_COUPLING       */
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
#include <tuple>

#include "dddi.h"
#include "if.h"

USING_UG_NAMESPACE

/* PPIF namespace: */
using namespace PPIF;

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* definition of exported variables                                         */
/*                                                                          */
/****************************************************************************/

IF_DEF theIF[MAX_IF];
int nIFs;


/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* Function:  sort_IFCouplings                                              */
/*                                                                          */
/* Purpose:   qsort procedure for sorting interface couplings.              */
/*            coupling list will be ordered according to:                   */
/*                1. processor number of represented object copy            */
/*                    (increasing order)                                    */
/*                2. direction of interface according to priorities         */
/*                    (increasing order)                                    */
/*                3. attr property of objects                               */
/*                    (decreasing order)                                    */
/*                4. global ids of objects                                  */
/*                    (increasing order)                                    */
/*                                                                          */
/* Input:     two couplings `a`, `b`                                        */
/*                                                                          */
/* Output:    `a < b`                                                       */
/*                                                                          */
/****************************************************************************/

static bool sort_IFCouplings (const COUPLING* a, const COUPLING* b)
{
  const auto aDir = CPLDIR(a), bDir = CPLDIR(b);
  // a, b switched in the third component as decreasing order is used for attr
  return std::tie(CPL_PROC(a), aDir, OBJ_ATTR(b->obj), OBJ_GID(a->obj))
       < std::tie(CPL_PROC(b), bDir, OBJ_ATTR(a->obj), OBJ_GID(b->obj));
}



void IFDeleteAll (DDD_IF ifId)
{
  IF_PROC  *ifh, *ifhNext;
  IF_ATTR *ifr, *ifrNext;

  /* free IF_PROC memory */
  ifh=theIF[ifId].ifHead;
  while (ifh!=NULL)
  {
    ifhNext = ifh->next;

    /* free IF_ATTR memory */
    ifr=ifh->ifAttr;
    while (ifr!=NULL)
    {
      ifrNext = ifr->next;
      delete ifr;
      ifr = ifrNext;
    }

    delete ifh;

    ifh = ifhNext;
  }

  /* free memory for coupling table */
  if (theIF[ifId].cpl!=NULL)
  {
    FreeIF(theIF[ifId].cpl);
    theIF[ifId].cpl=NULL;
  }

  /* free memory for shortcut object table */
  if (theIF[ifId].obj!=NULL)
  {
    FreeIF(theIF[ifId].obj);
    theIF[ifId].obj=NULL;
  }

  /* reset pointers */
  theIF[ifId].ifHead = NULL;
  theIF[ifId].nIfHeads = 0;
}




/* TODO  el-set relation, VERY inefficient! */
static bool is_elem (DDD_PRIO el, int n, DDD_PRIO *set)
{
  for(int i=0; i<n; i++)
    if (set[i]==el)
      return(true);

  return(false);
}



static RETCODE update_channels(DDD::DDDContext& context, DDD_IF ifId)
{
  IF_PROC *ifh;
  int i;
  DDD_PROC *partners = DDD_ProcArray(context);

  if (theIF[ifId].nIfHeads==0)
  {
    RET_ON_OK;
  }

  /* MarkHeap(); */

  for(i=0, ifh=theIF[ifId].ifHead; ifh!=NULL; i++, ifh=ifh->next)
  {
    partners[i] = ifh->proc;
  }

  if (! IS_OK(DDD_GetChannels(context, theIF[ifId].nIfHeads)))
  {
    RET_ON_ERROR;
  }

  for(ifh=theIF[ifId].ifHead; ifh!=NULL; ifh=ifh->next) {
    ifh->vc = VCHAN_TO(context, ifh->proc);
  }

  /* ReleaseHeap(); */


  RET_ON_OK;
}


/****************************************************************************/

/* collect couplings into interface array, for standard interface */

static COUPLING ** IFCollectStdCouplings (void)
{
  COUPLING **cplarray;
  int index, n;

  if (nCplItems==0)
  {
    return(NULL);
  }

  /* get memory for couplings inside STD_IF */
  cplarray = (COUPLING **) AllocIF(sizeof(COUPLING *)*nCplItems);
  if (cplarray==NULL) {
    DDD_PrintError('E', 4000, STR_NOMEM " in IFCreateFromScratch");
    HARD_EXIT;
  }

  /* collect couplings */
  n=0;
  for(index=0; index<NCpl_Get; index++)
  {
    COUPLING  *cpl;

    for(cpl=IdxCplList(index); cpl!=NULL; cpl=CPL_NEXT(cpl))
    {
      cplarray[n] = cpl;
      SETCPLDIR(cpl,0);
      n++;
    }
  }
  /*
     printf("%04d: n=%d, nCplItems=%d\n",me,n,nCplItems);
   */
  assert(n==nCplItems);
  return(cplarray);
}


/****************************************************************************/

static RETCODE IFCreateFromScratch(DDD::DDDContext& context, COUPLING **tmpcpl, DDD_IF ifId)
{
  IF_PROC     *ifHead = nullptr, *lastIfHead;
  IF_ATTR    *ifAttr = nullptr, *lastIfAttr = nullptr;
  int n, i;
  DDD_PROC lastproc;
  int STAT_MOD;

  const auto& objTable = context.objTable();

  STAT_GET_MODULE(STAT_MOD);
  STAT_SET_MODULE(DDD_MODULE_IF);

  /* first delete possible old interface */
  IFDeleteAll(ifId);

  STAT_RESET1;
  if (ifId==STD_INTERFACE)
  {
    theIF[ifId].cpl = IFCollectStdCouplings();
    n = nCplItems;
  }
  else
  {
    int index;


    /* collect relevant couplings into tmpcpl array */
    n=0;
    for(index=0; index<NCpl_Get; index++)
    {
      /* determine whether object belongs to IF */
      if ((1<<OBJ_TYPE(objTable[index])) & theIF[ifId].maskO)
      {
        const bool objInA = is_elem(OBJ_PRIO(objTable[index]),
                                    theIF[ifId].nPrioA, theIF[ifId].A);
        const bool objInB = is_elem(OBJ_PRIO(objTable[index]),
                                    theIF[ifId].nPrioB, theIF[ifId].B);

        if (objInA || objInB)
        {
          COUPLING  *cpl;

          /* test coupling list */
          for(cpl=IdxCplList(index); cpl!=NULL; cpl=CPL_NEXT(cpl))
          {
            const bool cplInA = is_elem(cpl->prio,
                                        theIF[ifId].nPrioA, theIF[ifId].A);
            const bool cplInB = is_elem(cpl->prio,
                                        theIF[ifId].nPrioB, theIF[ifId].B);

            /* compute possible IF directions */
            const int dir = ((objInA&&cplInB) ? DirAB : 0) |
                            ((objInB&&cplInA) ? DirBA : 0);

            if (dir != 0)
            {
              SETCPLDIR(cpl,dir);
              tmpcpl[n] = cpl;
              n++;
            }
          }
        }
      }
    }

    if (n>0)
    {
      /* re-alloc cpllist, now with correct size */
      theIF[ifId].cpl = (COUPLING **) AllocIF(sizeof(COUPLING *)*n);
      if (theIF[ifId].cpl==NULL)
      {
        sprintf(cBuffer, STR_NOMEM
                " for IF %02d in IFCreateFromScratch", ifId);
        DDD_PrintError('E', 4001, cBuffer);
        RET_ON_ERROR;
      }

      /* copy data from temporary array */
      memcpy((void *)theIF[ifId].cpl, (void *)tmpcpl, sizeof(COUPLING *)*n);
    }
    else
    {
      theIF[ifId].cpl = NULL;
    }
  }
  STAT_TIMER1(T_CREATE_COLLECT);



  /* sort IF couplings */
  STAT_RESET1;
  if (n>1)
    std::sort(theIF[ifId].cpl, theIF[ifId].cpl + n, sort_IFCouplings);
  STAT_TIMER1(T_CREATE_SORT);


  /* create IF_PROCs */
  STAT_RESET1;
  lastproc = PROC_INVALID;
  lastIfHead  = NULL;
  theIF[ifId].nIfHeads = 0;
  for(i=0; i<n; i++)
  {
    COUPLING  **cplp = &(theIF[ifId].cpl[i]);
    COUPLING  *cpl = *cplp;
    DDD_ATTR attr = OBJ_ATTR(cpl->obj);

    if (CPL_PROC(cpl) != lastproc)
    {
      /* create new IfHead */
      theIF[ifId].nIfHeads++;
      ifHead = new IF_PROC;
      ifHead->cpl      = cplp;
      ifHead->proc     = CPL_PROC(cpl);
      ifHead->next     = lastIfHead;
      lastIfHead = ifHead;
      lastproc   = ifHead->proc;

      ifHead->nAttrs = 1;
      ifHead->ifAttr = ifAttr = new IF_ATTR(attr);
      lastIfAttr = ifAttr;
    }

    /* count #items per processor */
    ifHead->nItems++;


    /* keep current ifAttr or find new one? */
    if (attr!=ifAttr->attr)
    {
      IF_ATTR *ifR;

      /* does ifAttr already exist? */
      ifR = ifHead->ifAttr;
      while ((ifR!=NULL) && (ifR->attr!=attr))
      {
        ifR=ifR->next;
      }

      if (ifR!=NULL)
      {
        /* reuse existing ifAttr */
        ifAttr = ifR;
      }
      else
      {
        /* create new ifAttr */
        ifHead->nAttrs++;
        ifAttr = new IF_ATTR(attr);
        lastIfAttr->next = ifAttr;
        lastIfAttr = ifAttr;
      }
    }


    /* count #items per processor and attr */
    ifAttr->nItems++;


    /* count #items per directions AB, BA or ABA
            and set beginnings of AB/BA/ABA subarrays */
    if (ifId!=STD_INTERFACE)
    {
      switch (CPLDIR(cpl))
      {
      case DirAB :
        ifHead->nAB++;
        if (ifHead->cplAB==0) ifHead->cplAB = cplp;
        ifAttr->nAB++;
        if (ifAttr->cplAB==0) ifAttr->cplAB = cplp;
        break;

      case DirBA :
        ifHead->nBA++;
        if (ifHead->cplBA==0) ifHead->cplBA = cplp;
        ifAttr->nBA++;
        if (ifAttr->cplBA==0) ifAttr->cplBA = cplp;
        break;

      case DirABA :
        ifHead->nABA++;
        if (ifHead->cplABA==0) ifHead->cplABA = cplp;
        ifAttr->nABA++;
        if (ifAttr->cplABA==0) ifAttr->cplABA = cplp;
        break;
      }
    }
  }
  STAT_TIMER1(T_CREATE_BUILD);

  /* remember anchor of ifHead list */
  if (theIF[ifId].nIfHeads>0) {
    theIF[ifId].ifHead = ifHead;
  }

  /* store overall number of coupling items */
  theIF[ifId].nItems = n;


  /* establish obj-table as an addressing shortcut */
  STAT_RESET1;
  IFCreateObjShortcut(context, ifId);
  STAT_TIMER1(T_CREATE_SHORTCUT);


  STAT_RESET1;
  if (! IS_OK(update_channels(context, ifId)))
  {
    DDD_PrintError('E', 4003, "couldn't create communication channels");
    RET_ON_ERROR;
  }
  STAT_TIMER1(T_CREATE_COMM);

  /* TODO das handling der VCs muss noch erheblich verbessert werden */
  /* TODO durch das is_elem suchen ist alles noch VERY inefficient */

  STAT_SET_MODULE(STAT_MOD);

  RET_ON_OK;
}


/****************************************************************************/
/*                                                                          */
/*  DDD_IFDefine                                                            */
/*                                                                          */
/****************************************************************************/

/**
        Definition of a DDD Interface.

        This function defines a new \ddd{interface}. Its argument list contains
        three arrays: the first one specifies a subset of the global DDD object set,
        the second and third ones specify a subset of all DDD priorities.
        After the initial creation of the new interface its ID is returned.

        During all following DDD operations ({\bf Identify}- as well as
        {\bf Transfer}-operations) the interface will be kept consistent, and can
        be used for communication via \funk{IFExchange} and
        \funk{IFOneway} and analogous functions.

   @param nO  number of entries in array {\it O[\ ]} (as specified with second argument).
   @param O   array of DDD type IDs; the new interface will only contain objects with one of these types.
   @param nA  number of entries in array {\it A[\ ]} (as specified with forth argument).
   @param A   first array of DDD priorities; the object set A from the new interface will only contain objects with one of these priorities.
   @param nB  number of entries in array {\it B[\ ]} (as specified with sixth argument).
   @param B   second array of DDD priorities; the object set B from the new interface will only contain objects with one of these priorities.
 */

DDD_IF DDD_IFDefine (
  DDD::DDDContext& context,
  int nO, DDD_TYPE O[],
  int nA, DDD_PRIO A[],
  int nB, DDD_PRIO B[])
{
int i;

if (nIFs==MAX_IF) {
  DDD_PrintError('E', 4100, "no more interfaces in DDD_IFDefine");
  return(0);
}

/* construct interface definition */
theIF[nIFs].nObjStruct = nO;
theIF[nIFs].nPrioA     = nA;
theIF[nIFs].nPrioB     = nB;
memcpy(theIF[nIFs].O, O, nO*sizeof(DDD_TYPE));
memcpy(theIF[nIFs].A, A, nA*sizeof(DDD_PRIO));
memcpy(theIF[nIFs].B, B, nB*sizeof(DDD_PRIO));
if (nO>1) std::sort(theIF[nIFs].O, theIF[nIFs].O + nO);
if (nA>1) std::sort(theIF[nIFs].A, theIF[nIFs].A + nA);
if (nB>1) std::sort(theIF[nIFs].B, theIF[nIFs].B + nB);

/* reset name string */
theIF[nIFs].name[0] = 0;

/* compute hash tables for fast access */
theIF[nIFs].maskO = 0;
for(i=0; i<nO; i++)
  theIF[nIFs].maskO |= (1<<(unsigned int)O[i]);


/* create initial interface state */
theIF[nIFs].ifHead = NULL;
if (nCplItems>0)
{
  /* allocate temporary cpl-list, this will be too large for
     average interfaces. */
  std::vector<COUPLING*> tmpcpl(nCplItems);

  if (! IS_OK(IFCreateFromScratch(context, tmpcpl.data(), nIFs)))
  {
    DDD_PrintError('E', 4101, "cannot create interface in DDD_IFDefine");
    return(0);
  }
}
else
{
  if (! IS_OK(IFCreateFromScratch(context, NULL, nIFs)))
  {
    DDD_PrintError('E', 4102, "cannot create interface in DDD_IFDefine");
    return(0);
  }
}


nIFs++;

return(nIFs-1);
}



static void StdIFDefine(DDD::DDDContext& context)
{
  /* exception: no OBJSTRUCT or priority entries */
  theIF[STD_INTERFACE].nObjStruct = 0;
  theIF[STD_INTERFACE].nPrioA     = 0;
  theIF[STD_INTERFACE].nPrioB     = 0;

  theIF[STD_INTERFACE].maskO = 0xffff;


  /* reset name string */
  theIF[nIFs].name[0] = 0;


  /* create initial interface state */
  theIF[STD_INTERFACE].ifHead = NULL;
  if (! IS_OK(IFCreateFromScratch(context, NULL, STD_INTERFACE)))
  {
    DDD_PrintError('E', 4104,
                   "cannot create standard interface during IF initialization");
    HARD_EXIT;
  }
}



void DDD_IFSetName(DDD::DDDContext&, DDD_IF ifId, const char *name)
{
/* copy name string */
strncpy(theIF[ifId].name, name, IF_NAMELEN-1);
}


/****************************************************************************/

void DDD_InfoIFImpl(DDD::DDDContext& context, DDD_IF ifId)
{
  IF_PROC    *ifh;

  sprintf(cBuffer, "|\n| DDD_IFInfoImpl for proc=%03d, IF %02d\n", me, ifId);
  DDD_PrintLine(cBuffer);

  sprintf(cBuffer, "|   cpl=%p  nIfHeads=%03d first=%p\n",
          theIF[ifId].cpl, theIF[ifId].nIfHeads, theIF[ifId].ifHead);
  DDD_PrintLine(cBuffer);

  for(ifh=theIF[ifId].ifHead; ifh!=NULL; ifh=ifh->next)
  {
    int i;

    sprintf(cBuffer, "|   head=%p cpl=%p p=%03d nItems=%05d nAttrs=%03d\n",
            ifh, ifh->cpl, ifh->proc, ifh->nItems, ifh->nAttrs);
    DDD_PrintLine(cBuffer);

    sprintf(cBuffer, "|      nAB= %05d\n", ifh->nAB);
    DDD_PrintLine(cBuffer);
    for(i=0; i<ifh->nAB; i++)
    {
      COUPLING *c = ifh->cplAB[i];
      sprintf(cBuffer, "|         gid=" OBJ_GID_FMT " proc=%04d prio=%02d "
              "osc=%p/%p\n",
              OBJ_GID(c->obj), CPL_PROC(c), c->prio,
              ifh->objAB[i], OBJ_OBJ(context, c->obj)
              );
      DDD_PrintLine(cBuffer);
    }

    sprintf(cBuffer, "|      nBA= %05d\n", ifh->nBA);
    DDD_PrintLine(cBuffer);
    for(i=0; i<ifh->nBA; i++)
    {
      COUPLING *c = ifh->cplBA[i];
      sprintf(cBuffer, "|         gid=" OBJ_GID_FMT " proc=%04d prio=%02d "
              "osc=%p/%p\n",
              OBJ_GID(c->obj), CPL_PROC(c), c->prio,
              ifh->objBA[i], OBJ_OBJ(context, c->obj)
              );
      DDD_PrintLine(cBuffer);
    }

    sprintf(cBuffer, "|      nABA=%05d\n", ifh->nABA);
    DDD_PrintLine(cBuffer);
    for(i=0; i<ifh->nABA; i++)
    {
      COUPLING *c = ifh->cplABA[i];
      sprintf(cBuffer, "|         gid=" OBJ_GID_FMT " proc=%04d prio=%02d "
              "osc=%p/%p\n",
              OBJ_GID(c->obj), CPL_PROC(c), c->prio,
              ifh->objABA[i], OBJ_OBJ(context, c->obj)
              );
      DDD_PrintLine(cBuffer);
    }
  }
  DDD_PrintLine("|\n");
}



static void IFDisplay (const DDD::DDDContext& context, DDD_IF i)
{
  IF_PROC    *ifh;
  IF_ATTR    *ifr;
  int j;
  char buf[50];

  sprintf(cBuffer, "| IF %02d ", i);
  if (i==STD_INTERFACE)
  {
    sprintf(buf, "including all (%08x)\n|       prio all to all\n",
            theIF[i].maskO);
    strcat(cBuffer, buf);
  }
  else
  {
    strcat(cBuffer, "including ");
    for(j=0; j<theIF[i].nObjStruct; j++)
    {
      sprintf(buf, "%s ", context.typeDefs()[theIF[i].O[j]].name);
      strcat(cBuffer, buf);
    }
    sprintf(buf, "(%08x)\n|       prio ", theIF[i].maskO);
    strcat(cBuffer, buf);

    for(j=0; j<theIF[i].nPrioA; j++)
    {
      sprintf(buf, "%d ", theIF[i].A[j]);
      strcat(cBuffer, buf);
    }
    strcat(cBuffer, "to ");
    for(j=0; j<theIF[i].nPrioB; j++)
    {
      sprintf(buf, "%d ", theIF[i].B[j]);
      strcat(cBuffer, buf);
    }
    strcat(cBuffer, "\n");
  }
  DDD_PrintLine(cBuffer);

  if (theIF[i].name[0]!=0)
  {
    sprintf(cBuffer, "|       '%s'\n", theIF[i].name);
    DDD_PrintLine(cBuffer);
  }

  for(ifh=theIF[i].ifHead; ifh!=NULL; ifh=ifh->next)
  {
    if (DDD_GetOption(OPT_INFO_IF_WITH_ATTR)==OPT_OFF)
    {
      sprintf(cBuffer, "|        %3d=%3d,%3d,%3d - %02d\n",
              ifh->nItems, ifh->nAB, ifh->nBA, ifh->nABA, ifh->proc);
      DDD_PrintLine(cBuffer);
    }
    else
    {
      sprintf(cBuffer, "|        %3d=%3d,%3d,%3d - %02d - #a=%05d\n",
              ifh->nItems, ifh->nAB, ifh->nBA, ifh->nABA,
              ifh->proc, ifh->nAttrs);
      DDD_PrintLine(cBuffer);

      for (ifr=ifh->ifAttr; ifr!=NULL; ifr=ifr->next)
      {
        sprintf(cBuffer, "|      a %3d=%3d,%3d,%3d - %04d\n",
                ifr->nItems, ifr->nAB, ifr->nBA, ifr->nABA, ifr->attr);
        DDD_PrintLine(cBuffer);
      }
    }
  }
}


/****************************************************************************/
/*                                                                          */
/*  DDD_IFDisplay                                                           */
/*                                                                          */
/****************************************************************************/

/**
        Display overview of single DDD interface.
        This function displays an overview table for one DDD-interface,
        its definition parameters and the current number of constituing objects
        on the calling processor.

        For each neighbour processor corresponding
        to that interface a relation line is displayed, which contains
        the overall number of objects inside the interface, the number of
        oneway relations outwards, the number of oneway relations inwards,
        the number of exchange relations and the neighbour processor number.

   @param aIF  the \ddd{interface} ID.
 */

void DDD_IFDisplay(const DDD::DDDContext& context, DDD_IF aIF)
{
if (aIF>=nIFs)
{
  sprintf(cBuffer, "invalid IF %02d in DDD_IFDisplay", aIF);
  DDD_PrintError('W', 4050, cBuffer);
  return;
}


sprintf(cBuffer, "|\n| DDD_IF-Info for proc=%03d\n", me);
DDD_PrintLine(cBuffer);

IFDisplay(context, aIF);

DDD_PrintLine("|\n");
}



/****************************************************************************/
/*                                                                          */
/*  DDD_IFDisplayAll                                                        */
/*                                                                          */
/****************************************************************************/

/**
        Display overview of all DDD interfaces.
        This function displays an overview table containing all DDD-interfaces,
        their definition parameters and the current number of constituing objects
        on the calling processor.

        For each interface and each neighbour processor corresponding
        to that interface a relation line is displayed, which contains
        the overall number of objects inside the interface, the number of
        oneway relations outwards, the number of oneway relations inwards,
        the number of exchange relations and the neighbour processor number.
 */

void DDD_IFDisplayAll(const DDD::DDDContext& context)
{
  int i;

  sprintf(cBuffer, "|\n| DDD_IF-Info for proc=%03d (all)\n", me);
  DDD_PrintLine(cBuffer);

  for(i=0; i<nIFs; i++)
  {
    IFDisplay(context, i);
  }

  DDD_PrintLine("|\n");
}



static void IFRebuildAll(DDD::DDDContext& context)
{
  /* create standard interface */
  if (! IS_OK(IFCreateFromScratch(context, NULL, STD_INTERFACE)))
  {
    DDD_PrintError('E', 4105,
                   "cannot create standard interface in IFRebuildAll");
    HARD_EXIT;
  }


  if (nIFs>1)
  {
    int i;

    if (nCplItems>0)
    {
      /* allocate temporary cpl-list, this will be too large for
              average interfaces. */
      std::vector<COUPLING*> tmpcpl(nCplItems);

      /* TODO: ausnutzen, dass STD_IF obermenge von allen interfaces ist */
      for(i=1; i<nIFs; i++)
      {
        if (! IS_OK(IFCreateFromScratch(context, tmpcpl.data(), i)))
        {
          sprintf(cBuffer, "cannot create interface %d in IFRebuildAll", i);
          DDD_PrintError('E', 4106, cBuffer);
          HARD_EXIT;
        }

        /*
           DDD_InfoIFImpl(context, i);
         */
      }
    }
    else
    {
      /* delete old interface structures */
      for(i=1; i<nIFs; i++)
      {
        /* delete possible old interface */
        IFDeleteAll(i);
      }
    }
  }
}


void IFAllFromScratch(DDD::DDDContext& context)
{
  if (DDD_GetOption(OPT_IF_CREATE_EXPLICIT)==OPT_ON)
  {
    /* interfaces must be created explicitly by calling
       DDD_IFRefreshAll(). This is for doing timings from
       application level. */
    return;
  }

  IFRebuildAll(context);
}



void DDD_IFRefreshAll(DDD::DDDContext& context)
{
  if (DDD_GetOption(OPT_IF_CREATE_EXPLICIT)==OPT_OFF)
  {
    /* if interfaces are not created explicit, then they
       are always kept consistent automatically. therefore,
       this function is senseless. */
    /* return;  nevertheless, dont return, create interfaces
                once more. just to be sure. */
  }

  IFRebuildAll(context);
}


/****************************************************************************/

void ddd_IFInit(DDD::DDDContext& context)
{
  /* init lists of unused items */
  theIF[0].ifHead = NULL;
  theIF[0].cpl    = NULL;

  /* init standard interface */
  StdIFDefine(context);

  /* no other interfaces yet */
  nIFs = 1;
}


void ddd_IFExit(DDD::DDDContext& context)
{
  int i;

  for(i=0; i<nIFs; i++)
    IFDeleteAll(i);
}


/****************************************************************************/


static size_t IFInfoMemory(const DDD::DDDContext&, DDD_IF ifId)
{
  IF_PROC *ifp;
  size_t sum=0;

  sum += sizeof(IF_PROC)    * theIF[ifId].nIfHeads;         /* component ifHead */
  sum += sizeof(COUPLING *) * theIF[ifId].nItems;           /* component cpl    */
  sum += sizeof(IFObjPtr)   * theIF[ifId].nItems;           /* component obj    */

  for(ifp=theIF[ifId].ifHead; ifp!=NULL; ifp=ifp->next)
  {
    sum += sizeof(IF_ATTR) * ifp->nAttrs;              /* component ifAttr */
  }

  return(sum);
}



size_t DDD_IFInfoMemory(const DDD::DDDContext& context, DDD_IF ifId)
{
if (ifId>=nIFs)
{
  sprintf(cBuffer, "invalid IF %02d in DDD_IFInfoMemory", ifId);
  DDD_PrintError('W', 4051, cBuffer);
  HARD_EXIT;
}

return(IFInfoMemory(context, ifId));
}


size_t DDD_IFInfoMemoryAll(const DDD::DDDContext& context)
{
  int i;
  size_t sum = 0;


  for(i=0; i<nIFs; i++)
  {
    sum += IFInfoMemory(context, i);
  }

  return(sum);
}

END_UGDIM_NAMESPACE

/****************************************************************************/
