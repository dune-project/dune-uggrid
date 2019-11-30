// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  pgmcheck.c													*/
/*																			*/
/* Purpose:   functions for checking parallel consistency                                       */
/*																			*/
/* Author:	  Stefan Lang                                                                   */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: birken@ica3.uni-stuttgart.de							*/
/*			  phone: 0049-(0)711-685-7003									*/
/*			  fax  : 0049-(0)711-685-7000									*/
/*																			*/
/* History:   980204 sl  begin                                                                                          */
/*																			*/
/* Remarks:                                                                                                                             */
/*																			*/
/****************************************************************************/

#ifdef ModelP

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <config.h>
#include <cstdlib>

#include "parallel.h"
#include <dune/uggrid/ugdevices.h>
#include <dune/uggrid/gm/evm.h>
#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/gm/refine.h>
#include <dune/uggrid/gm/shapes.h>
#include <dune/uggrid/gm/ugm.h>
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/general.h>
#include <dune/uggrid/low/namespace.h>

/* UG namespaces: */
USING_UG_NAMESPACES

/* PPIF namespace: */
using namespace PPIF;


/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

#define CHECK_OBJECT_PRIO(o,prio,master,ghost,id,s,_nerr_)                   \
  if (USED(o)==1 && ! master (o))                                          \
  {                                                                        \
    UserWriteF("MASTER %s=" id ## _FMTX " has WRONG prio=%d\n",      \
               s, id ## _PRTX(o),prio(o));                                  \
    _nerr_++;                                                        \
  }                                                                        \
  if (USED( o )==0 && ! ghost ( o ))                                       \
  {                                                                        \
    UserWriteF("GHOST %s=" id ## _FMTX " has WRONG prio=%d\n",       \
               s, id ## _PRTX( o ),prio(o));                                \
    _nerr_++;                                                        \
  }

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* definition of exported global variables									*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

REP_ERR_FILE


/* count for errors to report from communication scatter functions */
static INT check_distributed_objects_errors = 0;


/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*
   CheckProcListCons -

   SYNOPSIS:
   INT CheckProcListCons (int *proclist, int uniqueTag);

   PARAMETERS:
   .  proclist
   .  uniqueTag

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT NS_DIM_PREFIX CheckProcListCons (int *proclist, int uniqueTag)
{
  int nunique = 0;

  /* check uniqueness */
  while (*proclist != -1)
  {
    if (*(proclist+1) == uniqueTag) nunique++;
    proclist += 2;
  }

  /* nunique must be 1 for master elements   */
  /* nunique can  be 0/1 for (inner) nodes   */
  /*   with PrioBorder/PrioMaster            */
  return (nunique);
}



/****************************************************************************/
/*
   ListProcList -

   SYNOPSIS:
   INT ListProcList (int *proclist, int uniqueTag);

   PARAMETERS:
   .  proclist
   .  uniqueTag

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT ListProcList (int *proclist, int uniqueTag)
{
  while (*proclist != -1)
  {
    if (*(proclist+1) == uniqueTag)
      UserWriteF(" proc=%d",*proclist);
    proclist += 2;
  }
  return(0);
}


/****************************************************************************/
/*
   CheckVectorPrio -

   SYNOPSIS:
   INT CheckVectorPrio (ELEMENT *theElement, VECTOR *theVector);

   PARAMETERS:
   .  theElement
   .  theVector

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT CheckVectorPrio (DDD::DDDContext& context, ELEMENT *theElement, VECTOR *theVector)
{
  INT nmaster;
  INT nerrors = 0;

  /* check vector prio */
  CHECK_OBJECT_PRIO(theVector,PRIO,MASTER,GHOST,VINDEX,"Vector",nerrors)

  /* master copy has to be unique */
  if ((nmaster = CheckProcListCons(PROCLIST(context, theVector),PrioMaster)) > 1)
  {
    UserWriteF("VECTOR=" ID_FMTX " ERROR: master copy not unique, nmaster=%d:",
               ID_PRTX(theVector),nmaster);
    ListProcList(PROCLIST(context, theVector),PrioMaster);
    UserWriteF("\n");
    nerrors++;
  }

  return(nerrors);
}


/****************************************************************************/
/*
   CheckNodePrio -

   SYNOPSIS:
   INT CheckNodePrio (ELEMENT *theElement, NODE *theNode);

   PARAMETERS:
   .  theElement
   .  theNode

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

#ifdef __PERIODIC_BOUNDARY__
#define MAX_PERIODIC_PROCS      128

static INT CheckPerNodeVecPrio (DDD::DDDContext& context, NODE *theNode)
{
  INT nerrors = 0;
  VECTOR *vec = NVECTOR(theNode);
  int *vpl,*proclist = PROCLIST(context, theNode);
  int j,Proclist[MAX_PERIODIC_PROCS];

  /* copy proclist */
  j = 0;
  while (*proclist != -1)
  {
    Proclist[j] = proclist[0];
    Proclist[j+1] = proclist[1];

    proclist += 2;
    j+=2;
    assert(j < MAX_PERIODIC_PROCS);
  }
  Proclist[j] = -1;
  proclist = Proclist;

  /* get the vec proclist */
  vpl = PROCLIST(context, vec);

  /* compare both lists using criteria:          */
  /* 1. each proc from node proclist must also   */
  /* store vec (P(node) included in P(vec))      */
  /* 2. vec priority has to be greater than node */
  /*    prio, except the BorderPrio case!        */
  while (*proclist != -1)
  {
    INT proc = proclist[0];
    INT prio = proclist[1];

    while (*vpl != -1)
    {
      INT vproc = vpl[0];
      INT vprio = vpl[1];

      if (proc == vproc)
      {
        if (prio > vprio && GHOSTPRIO(vprio))
        {
          UserWriteF(PFMT "Vec=" VINDEX_FMTX " Node=" ID_FMTX
                     ": ERROR proclist mismatch in PRIO for proc=%d prio=%d!\n",
                     me,VINDEX_PRTX(vec),ID_PRTX(theNode),proc,prio);
          nerrors++;
        }
        /* found */
        break;
      }

      vpl += 2;
    }
    if (*vpl == -1)
    {
      UserWriteF(PFMT "Vec=" VINDEX_FMTX " Node=" ID_FMTX
                 ": ERROR proclist mismatch in PROC for proc=%d prio=%d!\n",
                 me,VINDEX_PRTX(vec),ID_PRTX(theNode),proc,prio);
      nerrors++;
    }

    proclist += 2;
  }

  return(nerrors);
}
#endif

static INT CheckNodePrio (DDD::DDDContext& context, ELEMENT *theElement, NODE *theNode)
{
  INT nmaster;
  INT nerrors = 0;

  /* check node prio */
  CHECK_OBJECT_PRIO(theNode,PRIO,MASTER,GHOST,ID,"NODE",nerrors)

  /* master copy has to be unique */
    if ((nmaster = CheckProcListCons(PROCLIST(context, theNode),PrioMaster)) > 1)
  {
    UserWriteF("NODE=" ID_FMTX " ERROR: master copy not unique, nmaster=%d:",
               ID_PRTX(theNode),nmaster);
    ListProcList(PROCLIST(context, theNode),PrioMaster);
    UserWriteF("\n");
    nerrors++;
  }

  if (ddd_ctrl(context).nodeData)
  {
    if (NVECTOR(theNode) != NULL)
    {
      nerrors += CheckVectorPrio(context, theElement,NVECTOR(theNode));
    }
#ifdef __PERIODIC_BOUNDARY__
    if (PRIO(theNode) > PRIO(NVECTOR(theNode)) && GHOSTPRIO(PRIO(NVECTOR(theNode))))
    {
      UserWriteF("NODE=" ID_FMTX " ERROR: WRONG PRIO of VEC" VINDEX_FMTX,
                 ID_PRTX(theNode),VINDEX_PRTX(NVECTOR(theNode)));
      nerrors++;
    }
    /* compare proclists of vector and node */
    nerrors += CheckPerNodeVecPrio(theNode);
#endif
  }

  return(nerrors);
}


/****************************************************************************/
/*
   CheckEdgePrio -

   SYNOPSIS:
   INT CheckEdgePrio (ELEMENT *theElement, EDGE *theEdge);

   PARAMETERS:
   .  theElement
   .  theEdge

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT CheckEdgePrio (DDD::DDDContext& context, ELEMENT *theElement, EDGE *theEdge)
{
  INT nerrors = 0;

        #ifdef __THREEDIM__
  INT nmaster;

  /* check edge prio */
  CHECK_OBJECT_PRIO(theEdge,PRIO,MASTER,GHOST,ID,"EDGE",nerrors)

  /* master copy has to be unique */
  if ((nmaster = CheckProcListCons(PROCLIST(context, theEdge),PrioMaster)) > 1)
  {
    UserWriteF("EDGE=" EDID_FMTX " ERROR: master copy not unique, nmaster=%d:",
               EDID_PRTX(theEdge),nmaster);
    ListProcList(PROCLIST(context, theEdge),PrioMaster);
    UserWriteF("\n");
    nerrors++;
  }
        #endif

  if (ddd_ctrl(context).edgeData)
    if (EDVECTOR(theEdge) != NULL)
      nerrors += CheckVectorPrio(context, theElement,EDVECTOR(theEdge));

  return(nerrors);
}


/****************************************************************************/
/*
   CheckElementPrio -

   SYNOPSIS:
   INT CheckElementPrio (ELEMENT *theElement);

   PARAMETERS:
   .  theElement

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static INT CheckElementPrio (DDD::DDDContext& context, ELEMENT *theElement)
{
  INT i,nmaster,prio,valid_copy;
  INT nerrors = 0;
  NODE    *theNode;
  EDGE    *theEdge;
  ELEMENT *SonList[MAX_SONS];

  const auto& me = context.me();

  if (PARTITION(theElement)==me && !EMASTER(theElement))
  {
    UserWriteF(PFMT "#FATAL# MASTER ELEM=" EID_FMTX " has WRONG part=%d prio=%d\n",
               me,EID_PRTX(theElement),PARTITION(theElement),EPRIO(theElement));
    nerrors++;
  }
  if (PARTITION(theElement)!=me && !EGHOST(theElement))
  {
    UserWriteF(PFMT "#FATAL# GHOST ELEM=" EID_FMTX " has WRONG part=%d prio=%d\n",
               me,EID_PRTX(theElement),PARTITION(theElement),EPRIO(theElement));
    nerrors++;

    /* test ghost prio */
    prio = 0;
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      if (EMASTER(NBELEM(theElement,i))) prio = PrioHGhost;
    }
    if (GetSons(theElement,SonList) != 0) RETURN(1);
    if (SonList[0] != NULL) prio += PrioVGhost;

    if (EPRIO(theElement) != prio)
    {
      UserWriteF(PFMT "ERROR GHOST ELEM=" EID_FMTX
                 " has WRONG prio=%d should be prio=%d\n",
                 me,EID_PRTX(theElement),EPRIO(theElement),prio);
      nerrors++;
    }
  }

  /* check element prio */
  CHECK_OBJECT_PRIO(theElement,EPRIO,EMASTER,EGHOST,EID,"ELEM",nerrors)

  /* master copy has to be unique */
  if ((nmaster = CheckProcListCons(EPROCLIST(context, theElement),PrioMaster)) != 1)
  {
    UserWriteF("ELEM=" EID_FMTX " ERROR: master copy not unique, ",
               EID_PRTX(theElement),nmaster);
    if (EFATHER(theElement) != NULL)
      UserWriteF("Father=" EID_FMTX, EID_PRTX(EFATHER(theElement)));
    else
      UserWrite("Father=NULL");
    UserWriteF(" nmaster=%d:",nmaster);
    ListProcList(EPROCLIST(context, theElement),PrioMaster);
    UserWriteF("\n");
    nerrors++;
  }

  /* hghost copy needs to a master neighbor */
  if (EHGHOST(theElement))
  {
    valid_copy = 0;
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
    {
      if (NBELEM(theElement,i)!=NULL && EMASTER(NBELEM(theElement,i)))
        valid_copy = 1;
    }
    if (!valid_copy)
    {
      UserWriteF("ELEM=" EID_FMTX " ERROR: hghost copy with no master neighbor!\n",
                 EID_PRTX(theElement));
      nerrors++;
    }
  }

  /* vghost copy needs to a master Son */
  if (EVGHOST(theElement))
  {
    if (GetSons(theElement,SonList) != 0) RETURN(1);
    if (SonList[0] == NULL) valid_copy = 0;
    else valid_copy = 1;
    if (!valid_copy)
    {
      UserWriteF("ELEM=" EID_FMTX " ERROR: vghost copy with no master son!\n",
                 EID_PRTX(theElement));
      nerrors++;
    }
  }

  if (ddd_ctrl(context).elemData)
    if (EVECTOR(theElement) != NULL)
      nerrors += CheckVectorPrio(context, theElement,EVECTOR(theElement));

  if (ddd_ctrl(context).sideData)
  {
    for (i=0; i<SIDES_OF_ELEM(theElement); i++)
      if (SVECTOR(theElement,i) != NULL)
        nerrors += CheckVectorPrio(context, theElement,SVECTOR(theElement,i));
  }

  for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    theNode = CORNER(theElement,i);
    nerrors += CheckNodePrio(context, theElement,theNode);
  }

  for (i=0; i<EDGES_OF_ELEM(theElement); i++)
  {
    theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),
                      CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
    ASSERT(theEdge != NULL);
    nerrors += CheckEdgePrio(context, theElement,theEdge);
  }

  return (nerrors);
}


/****************************************************************************/
/*
   CheckDistributedObjects - Check the uniqueness of global ids

   SYNOPSIS:
   INT CheckDistributedObjects (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:
   Compare the global ids of distributed objects which are identified.
   This is done for nodes and edges (3D).

   RETURN VALUE:
   INT
 */
/****************************************************************************/

static int Gather_ElemObjectGids (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  /* copy node gids into buffer */
  for (INT i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    ((DDD_GID *)data)[i] = GID(CORNER(theElement,i));
  }

        #ifdef __THREEDIM__
  /* copy edge gids into buffer */
  for (INT i=CORNERS_OF_ELEM(theElement),j=0; i<EDGES_OF_ELEM(theElement); i++,j++)
  {
    EDGE *theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement,j,0),
                            CORNER_OF_EDGE_PTR(theElement,j,1));
    assert(theEdge!=NULL);
    ((DDD_GID *)data)[i] = GID(theEdge);
  }
        #endif

  return 0;
}

static int Scatter_ElemObjectGids (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  ELEMENT *theElement = (ELEMENT *)obj;

  /* compare node gids with buffer gids */
  for (INT i=0; i<CORNERS_OF_ELEM(theElement); i++)
  {
    NODE* theNode = CORNER(theElement,i);
    if (((DDD_GID *)data)[i] != GID(theNode))
    {
      UserWriteF("ELEM=" EID_FMTX " #ERROR#: NODE=" ID_FMTX " gids don't match "
                 "local=%08x remote=%08x remoteproc/prio=%d/%d\n",EID_PRTX(theElement),ID_PRTX(theNode),
                 GID(theNode),((DDD_GID *)data)[i],proc,prio);
      check_distributed_objects_errors++;
      assert(0);
    }
  }

        #ifdef __THREEDIM__
  /* compare edge gids with buffer gids */
  for (INT i=CORNERS_OF_ELEM(theElement),j=0; i<EDGES_OF_ELEM(theElement); i++,j++)
  {
    EDGE* theEdge = GetEdge(CORNER_OF_EDGE_PTR(theElement,j,0),
                            CORNER_OF_EDGE_PTR(theElement,j,1));
    assert(theEdge!=NULL);
    if (((DDD_GID *)data)[i] != GID(theEdge))
    {
      UserWriteF("ELEM=" EID_FMTX " #ERROR#: EDGE=" ID_FMTX " gids don't match "
                 "local=%08x remote=%08x remoteproc/prio=%d/%d\n",EID_PRTX(theElement),ID_PRTX(theEdge),
                 GID(theEdge),((DDD_GID *)data)[i],proc,prio);
      check_distributed_objects_errors++;
      assert(0);
    }
  }
        #endif

  return 0;
}

#ifdef __THREEDIM__
static int Gather_EdgeObjectGids (DDD::DDDContext&, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  INT i;
  EDGE *theEdge = (EDGE *)obj;
  NODE *theNode0, *theNode1, *MidNode;

  i = 0;

  theNode0 = NBNODE(LINK0(theEdge));
  theNode1 = NBNODE(LINK1(theEdge));
  MidNode  = MIDNODE(theEdge);

  /* copy node gids into buffer */
  ((DDD_GID *)data)[i++] = GID(theNode0);
  ((DDD_GID *)data)[i++] = GID(theNode1);
  if (MidNode != NULL)
    ((DDD_GID *)data)[i++] = GID(MidNode)+1;
  else
    ((DDD_GID *)data)[i++] = 0;

  return 0;
}

static int Scatter_EdgeObjectGids (DDD::DDDContext& context, DDD_OBJ obj, void *data, DDD_PROC proc, DDD_PRIO prio)
{
  INT i;
  DDD_GID remotegid;
  EDGE *theEdge = (EDGE *)obj;
  NODE *theNode0, *theNode1, *MidNode;
  int *proclist = PROCLIST(context, theEdge);

  /* this check allows no edges copies of type VGHOST */
  /* since then midnode might be NULL due to local    */
  /* load balancing situation                         */
  while (*proclist != -1)
  {
    if (VGHOSTPRIO(*(proclist+1))) return(0);
    proclist += 2;
  }

  i = 0;

  theNode0 = NBNODE(LINK0(theEdge));
  theNode1 = NBNODE(LINK1(theEdge));
  MidNode  = MIDNODE(theEdge);

  /* compare node0 gids with buffer gids */
  if (((DDD_GID *)data)[i] != GID(theNode0))
  {
    UserWriteF("EDGE=" ID_FMTX " #ERROR#: NODE0=" ID_FMTX " gids don't match "
               "local=%08x remote=%08x remoteproc/prio=%d/%d\n",
               ID_PRTX(theEdge),ID_PRTX(theNode0),
               GID(theNode0),((DDD_GID *)data)[i],proc,prio);
    check_distributed_objects_errors++;
    assert(0);
  }
  i++;

  /* compare node1 gids with buffer gids */
  if (((DDD_GID *)data)[i] != GID(theNode1))
  {
    UserWriteF("EDGE=" ID_FMTX " #ERROR#: NODE1=" ID_FMTX " gids don't match "
               "local=%08x remote=%08x remoteproc/prio=%d/%d\n",
               ID_PRTX(theEdge),ID_PRTX(theNode1),
               GID(theNode1),((DDD_GID *)data)[i],proc,prio);
    check_distributed_objects_errors++;
    assert(0);
  }
  i++;

  /* compare node0 gids with buffer gids */
  if (((DDD_GID *)data)[i]>0)
    remotegid = ((DDD_GID *)data)[i]-1;

  if (MidNode != NULL)
  {
    if (remotegid != GID(MidNode))
    {
      UserWriteF("EDGE=" ID_FMTX " #ERROR#: MIDNODE=" ID_FMTX " gids don't match "
                 "local=%08x remote=%08x remoteproc/prio=%d/%d\n",
                 ID_PRTX(theEdge),ID_PRTX(MidNode),
                 GID(MidNode),remotegid,proc,prio);
      check_distributed_objects_errors++;
      assert(0);
    }
  }
  else
  {

    if (remotegid != 0)
    {
      UserWriteF("EDGE=" ID_FMTX " #ERROR#: MIDNODE=NULL gids don't match "
                 "local=%08x remote=%08x remoteproc/prio=%d/%d\n",
                 ID_PRTX(theEdge),0,remotegid,proc,prio);
      check_distributed_objects_errors++;
      assert(0);
    }
  }
  i++;

  return 0;
}
#endif

static INT CheckDistributedObjects (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  const auto& dddctrl = ddd_ctrl(context);

  INT nerrors;
        #ifdef __TWODIM__
  INT size = MAX_CORNERS_OF_ELEM;       /* compare the 3/4 node ids */
        #endif
        #ifdef __THREEDIM__
  INT size = MAX_CORNERS_OF_ELEM+MAX_EDGES_OF_ELEM;             /* compare 8 nodes + 12 edges */
        #endif

  check_distributed_objects_errors = 0;

  // void DDD_IFAOnewayX (DDD::DDDContext&, DDD_IF, DDD_ATTR, DDD_IF_DIR, size_t, ComProcXPtr, ComProcXPtr);
  DDD_IFAOnewayX(context,
                 dddctrl.ElementSymmVHIF,GRID_ATTR(theGrid),IF_BACKWARD,size*sizeof(DDD_GID),
                 Gather_ElemObjectGids, Scatter_ElemObjectGids);

        #ifdef __THREEDIM__
  if (0)
    DDD_IFAOnewayX(context,
                   dddctrl.BorderEdgeSymmIF,GRID_ATTR(theGrid),IF_BACKWARD,3*sizeof(DDD_GID),
                   Gather_EdgeObjectGids, Scatter_EdgeObjectGids);
        #endif

  nerrors = check_distributed_objects_errors;
  return(nerrors);
}

/****************************************************************************/
/*
   CheckInterfaces -

   SYNOPSIS:
   INT CheckInterfaces (GRID *theGrid);

   PARAMETERS:
   .  theGrid

   DESCRIPTION:

   RETURN VALUE:
   INT
 */
/****************************************************************************/

INT NS_DIM_PREFIX CheckInterfaces (GRID *theGrid)
{
  auto& context = theGrid->dddContext();
  auto& dddctrl = ddd_ctrl(context);

  INT i,j;
  ELEMENT *theElement;
  NODE    *theNode;
  EDGE    *theEdge;
  int nerrors = 0;

  /* reset USED flag of all grid objects  */
  /* set USED flag of master grid objects */
  for (j=0; j<2; j++)
  {
    for (theElement =(j==0 ? PFIRSTELEMENT(theGrid) : FIRSTELEMENT(theGrid));
         theElement!=NULL;
         theElement=SUCCE(theElement))
    {
      SETUSED(theElement,j);
      if (dddctrl.elemData)
        if (EVECTOR(theElement) != NULL)
          SETUSED(EVECTOR(theElement),j);
      if (dddctrl.sideData)
      {
        for (i=0; i<SIDES_OF_ELEM(theElement); i++)
          if (SVECTOR(theElement,i) != NULL)
            SETUSED(SVECTOR(theElement,i),j);
      }

      for (i=0; i<CORNERS_OF_ELEM(theElement); i++)
      {
        theNode = CORNER(theElement,i);
        SETUSED(theNode,j);
        if (dddctrl.nodeData)
          if (NVECTOR(theNode) != NULL)
            SETUSED(NVECTOR(theNode),j);
        SETUSED(MYVERTEX(theNode),j);
      }

      for (i=0; i<EDGES_OF_ELEM(theElement); i++)
      {
        theEdge = GetEdge(CORNER(theElement,CORNER_OF_EDGE(theElement,i,0)),
                          CORNER(theElement,CORNER_OF_EDGE(theElement,i,1)));
        ASSERT(theEdge != NULL);
        SETUSED(theEdge,j);
        if (dddctrl.edgeData)
          if (EDVECTOR(theEdge) != NULL)
            SETUSED(EDVECTOR(theEdge),j);
      }
    }
  }

  /* check validity of priorities */
  for (theElement=PFIRSTELEMENT(theGrid); theElement!=NULL;
       theElement=SUCCE(theElement))
  {
    nerrors += CheckElementPrio(theGrid->dddContext(), theElement);
  }

  /* check uniqueness of global ids for distributed nodes and edges */
  nerrors += CheckDistributedObjects(theGrid);

  /* check ddd interface consistency */
  DDD_SetOption(theGrid->dddContext(), OPT_QUIET_CONSCHECK, OPT_ON);
  nerrors += DDD_ConsCheck(theGrid->dddContext());
  DDD_SetOption(theGrid->dddContext(), OPT_QUIET_CONSCHECK, OPT_OFF);

  return(nerrors);
}

#endif
