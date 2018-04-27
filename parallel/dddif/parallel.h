// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                                                                                                      */
/* File:          parallel.h                                                                                                    */
/*                                                                                                                                                      */
/* Purpose:   defines for parallel ugp version 3                                                        */
/*                                                                                                                                                      */
/* Author:        Stefan Lang, Klaus Birken                                                             */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70550 Stuttgart                                                                                               */
/*                        email: stefan@ica3.uni-stuttgart.de                                                   */
/*                        phone: 0049-(0)711-685-7003                                                                   */
/*                        fax  : 0049-(0)711-685-7000                                                                   */
/*                                                                                                                                                      */
/* History:   09.05.95 begin, ugp version 3.0                                                           */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/


/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __PARALLEL_H__
#define __PARALLEL_H__

#include <memory>

#ifdef ModelP
#  include <dune/uggrid/parallel/ddd/dddcontext.hh>
#endif

#include "heaps.h"

#ifdef ModelP
#include "ppif.h"
#include "ddd.h"
#endif

#include "gm.h"
#include "pargm.h"

#include "namespace.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                                                                                                      */
/* defines in the following order                                                                                       */
/*                                                                                                                                                      */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef DUNE_UGGRID_HAVE_DDDCONTEXT
#  define DUNE_UGGRID_HAVE_DDDCONTEXT 1
#endif

#define MAXDDDTYPES   32

enum HandlerSets
{
  HSET_XFER = 0,
  HSET_REFINE
};


#ifdef ModelP

/* CE for nodes */
/* not used, kb 961216
 #define KEEP_VECTOR  0*/ /* this is a node with vector */
/*#define DEL_VECTOR  1*/  /* this is a node without vector */

/* status output from different parallel phases */
/*
   #define STAT_OUT
 */

/* define DDD_PRIO_ENV to change priority using fast PrioBegin/End env. */
/*
   #define DDD_PRIO_ENV
 */
#ifdef DDD_PRIO_ENV
#define DDD_PrioritySet(context, h,p) {ObjectPriorityUpdate(context, (DDD_OBJ)h,p); DDD_PrioChange(context, h,p);}
#endif

#define UGTYPE(context, t)     (ddd_ctrl(context).ugtypes[(t)])
#define DDDTYPE(context, t)    (ddd_ctrl(context).types[(t)])
#define HAS_DDDHDR(context, t) (ddd_ctrl(context).dddObj[(t)])

#define DDD_DOMAIN_DATA    DDD_USER_DATA+1
#define DDD_EXTRA_DATA     DDD_USER_DATA+2

/* macros for ddd object info */
/* for elements */
#define EPRIO(e)                                                (Priorities)DDD_InfoPriority(PARHDRE(e))
#define SETEPRIO(context, e,p)                          DDD_PrioritySet(context, PARHDRE(e),p)
#define SETEPRIOX(context, e,p)                         if (EPRIO(e)!=p) DDD_PrioritySet(context, PARHDRE(e),p)
#define EMASTER(e)                                              (EPRIO(e) == PrioMaster)
#define EGHOST(e)                                               (EPRIO(e)==PrioHGhost  || EPRIO(e)==PrioVGhost ||\
                                                                 EPRIO(e)==PrioVHGhost)
#define EVHGHOST(e)                                             (EPRIO(e)==PrioVHGhost)
#define EVGHOST(e)                                              (EPRIO(e)==PrioVGhost || EPRIO(e)==PrioVHGhost)
#define EHGHOST(e)                                              (EPRIO(e)==PrioHGhost  || EPRIO(e)==PrioVHGhost)
#define EGID(e)                                                 DDD_InfoGlobalId(PARHDRE(e))
#define EPROCLIST(context, e)                           DDD_InfoProcList(context, PARHDRE(e))
#define EPROCPRIO(context, e,p)                         DDD_InfoProcPrio(context, PARHDRE(e),p)
#define ENCOPIES(context, e)                                    DDD_InfoNCopies(context, PARHDRE(e))
#define EATTR(e)                                                DDD_InfoAttr(PARHDRE(e))
#define XFEREDELETE(context, e)                         DDD_XferDeleteObj(context, PARHDRE(e))
#define XFERECOPY(context, e,dest,prio)                 DDD_XferCopyObj(context, PARHDRE(e),dest,prio)
#define XFERECOPYX(context, e,dest,prio,size)           DDD_XferCopyObjX(context, PARHDRE(e),dest,prio,size)

/* for nodes, vectors, edges (edges only 3D) */
#define PRIO(e)                                                 DDD_InfoPriority(PARHDR(e))
#define SETPRIO(context, e,p)                           DDD_PrioritySet(context, PARHDR(e),p)
#define SETPRIOX(context, e,p)                          if (PRIO(e)!=p) DDD_PrioritySet(context, PARHDR(e),p)
#define MASTER(e)                                               (PRIO(e)==PrioMaster || PRIO(e)==PrioBorder)
#define GHOST(e)                                                (PRIO(e)==PrioHGhost  || PRIO(e)==PrioVGhost ||\
                                                                 PRIO(e)==PrioVHGhost)
#define VHGHOST(e)                                              (PRIO(e)==PrioVHGhost)
#define VGHOST(e)                                               (PRIO(e)==PrioVGhost || PRIO(e)==PrioVHGhost)
#define HGHOST(e)                                               (PRIO(e)==PrioHGhost  || PRIO(e)==PrioVHGhost)
#define GID(e)                                                  DDD_InfoGlobalId(PARHDR(e))
#define PROCLIST(context, e)                                    DDD_InfoProcList(context, PARHDR(e))
#define PROCPRIO(context, e,p)                          DDD_InfoProcPrio(context, PARHDR(e),p)
#define NCOPIES(context, e)                                     DDD_InfoNCopies(context, PARHDR(e))
#define ATTR(e)                                                 DDD_InfoAttr(PARHDR(e))
#define XFERDELETE(context, e)                          DDD_XferDeleteObj(context, PARHDR(e))
#define XFERCOPY(context, e,dest,prio)                  DDD_XferCopyObj(context, PARHDR(e),dest,prio)
#define XFERCOPYX(context, e,dest,prio,size)            DDD_XferCopyObjX(context, PARHDR(e),dest,prio,size)

/* for vertices */
#define VXPRIO(e)                                               DDD_InfoPriority(PARHDRV(e))
#define SETVXPRIO(context, e,p)                         DDD_PrioritySet(context, PARHDRV(e),p)
#define SETVXPRIOX(context, e,p)                        if (VXPRIO(e)!=p) DDD_PrioritySet(context, PARHDRV(e),p)
#define VXMASTER(e)                                             (VXPRIO(e)==PrioMaster || VXPRIO(e)==PrioBorder)
#define VXGHOST(e)                                              (VXPRIO(e)==PrioHGhost || VXPRIO(e)==PrioVGhost ||\
                                                                 VXPRIO(e)==PrioVHGhost)
#define VXVHGHOST(e)                                    (VXPRIO(e)==PrioVHGhost)
#define VXVGHOST(e)                                             (VXPRIO(e)==PrioVGhost || VXPRIO(e)==PrioVHGhost)
#define VXHGHOST(e)                                             (VXPRIO(e)==PrioHGhost || VXPRIO(e)==PrioVHGhost)
#define VXGID(e)                                                DDD_InfoGlobalId(PARHDRV(e))
#define VXPROCLIST(context, e)                          DDD_InfoProcList(context, PARHDRV(e))
#define VXPROCPRIO(context, e,p)                        DDD_InfoProcPrio(context, PARHDRV(e),p)
#define VXNCOPIES(context, e)                           DDD_InfoNCopies(context, PARHDRV(e))
#define VXATTR(e)                                               DDD_InfoAttr(PARHDRV(e))
#define XFERVXDELETE(context, e)                        DDD_XferDeleteObj(context, PARHDRV(e))
#define XFERVXCOPY(context, e,dest,prio)                DDD_XferCopyObj(context, PARHDRV(e),dest,prio,size)
#define XFERVXCOPYX(context, e,dest,prio,size)          DDD_XferCopyObjX(context, PARHDRV(e),dest,prio,size)

/* macros for priorities */
/* for elements */
#define EMASTERPRIO(p)                                  (p==PrioMaster)
#define EGHOSTPRIO(p)                                   (p==PrioHGhost || p==PrioVGhost || p==PrioVHGhost)
#define EVHGHOSTPRIO(p)                                 (p==PrioVHGhost)
#define EVGHOSTPRIO(p)                                  (p==PrioVGhost || p==PrioVHGhost)
#define EHGHOSTPRIO(p)                                  (p==PrioHGhost  || p==PrioVHGhost)

/* for nodes, vertices, vectors, edges (edges only 3D) */
#define MASTERPRIO(p)                                   (p==PrioMaster || p==PrioBorder)
#define GHOSTPRIO(p)                                    (p==PrioHGhost || p==PrioVGhost || p==PrioVHGhost)
#define VHGHOSTPRIO(p)                                  (p==PrioVHGhost)
#define VGHOSTPRIO(p)                                   (p==PrioVGhost || p==PrioVHGhost)
#define HGHOSTPRIO(p)                                   (p==PrioHGhost  || p==PrioVHGhost)

#define EGID_FMT DDD_GID_FMT
#define GID_FMT                                                 DDD_GID_FMT

/* This exchanges in the load balancing the connections too.
 #define __EXCHANGE_CONNECTIONS__                              */

#endif /* ModelP */


/****************************************************************************/
/*                                                                                                                                                      */
/* data structures exported by the corresponding source file                            */
/*                                                                                                                                                      */
/****************************************************************************/


/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported global variables                                                                      */
/*                                                                                                                                                      */
/****************************************************************************/

#ifdef ModelP
/* DDD objects */
extern DDD_TYPE TypeVector;
extern DDD_TYPE TypeIVertex, TypeBVertex;
extern DDD_TYPE TypeNode;

extern DDD_TYPE TypeUnknown;

#ifdef __TWODIM__
extern DDD_TYPE TypeTrElem, TypeTrBElem,
                TypeQuElem, TypeQuBElem;
#endif

#ifdef __THREEDIM__
extern DDD_TYPE TypeTeElem, TypeTeBElem;
extern DDD_TYPE TypePyElem, TypePyBElem;
extern DDD_TYPE TypePrElem, TypePrBElem;
extern DDD_TYPE TypeHeElem, TypeHeBElem;
#endif

/* DDD data objects */
extern DDD_TYPE TypeMatrix;
extern DDD_TYPE TypeBndP;
extern DDD_TYPE TypeEdge;
extern DDD_TYPE TypeBndS;

/* DDD Interfaces */
extern DDD_IF ElementIF, ElementSymmIF, ElementVIF, ElementSymmVIF,
              ElementVHIF, ElementSymmVHIF;
extern DDD_IF BorderNodeIF, BorderNodeSymmIF, OuterNodeIF, NodeVIF,
              NodeIF, NodeAllIF;
extern DDD_IF BorderVectorIF, BorderVectorSymmIF,
              OuterVectorIF, OuterVectorSymmIF,
              VectorVIF, VectorVAllIF, VectorIF;
extern DDD_IF EdgeIF, BorderEdgeSymmIF, EdgeHIF, EdgeVHIF,
              EdgeSymmVHIF;

/* DDD Global Controls */
struct DDD_CTRL
{
  /* data from ug */
  MULTIGRID *currMG;
  FORMAT    *currFormat;
  int nodeData;
  int edgeData;
  int elemData;
  int sideData;

  INT ugtypes[MAXDDDTYPES];                  /* dddtype -> ugtype */
  DDD_TYPE types[MAXOBJECTS];                /* ugtype -> dddtype */
  bool dddObj[MAXOBJECTS];

  /* status of DDDIF */
  bool allTypesDefined;
};

extern DDD_CTRL dddctrl;

#endif

/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

#ifdef ModelP

/**
 * accessor function for DDD user context
 */
inline
const DDD_CTRL& ddd_ctrl(const DDD::DDDContext& context)
{
  return dddctrl;
  // return *static_cast<const DDD_CTRL*>(context.data());
}

/**
 * accessor function for DDD user context
 */
inline
DDD_CTRL& ddd_ctrl(DDD::DDDContext& context)
{
  return dddctrl;
  // return *static_cast<DDD_CTRL*>(context.data());
}

/* from initddd.c */
int             InitDDD(DDD::DDDContext& context);
int             ExitDDD(DDD::DDDContext& context);
void    InitCurrMG              (MULTIGRID *);

/* from debugger.c */
void    ddd_pstat                       (DDD::DDDContext& context, char *);

/* from lb.c */
void lbs (const char *argv, MULTIGRID *theMG);

/* from handler.c */
void            ddd_HandlerInit                 (DDD::DDDContext& context, INT);
DDD_TYPE        NFatherObjType                  (DDD::DDDContext& context, DDD_OBJ obj, DDD_OBJ ref);
void            ObjectPriorityUpdate    (DDD::DDDContext& context, DDD_OBJ obj, DDD_PRIO newPrio);

/* from lbrcb.c */
int BalanceGridRCB (MULTIGRID *, int);

/* from gridcons.c */
void    ConstructConsistentGrid                 (GRID *theGrid);
void    ConstructConsistentMultiGrid    (MULTIGRID *theMG);

/* from pgmcheck.c */
INT                     CheckProcListCons (int *proclist, int uniqueTag);
INT             CheckInterfaces                         (GRID *theGrid);

/* from priority.c */
INT             SetGridBorderPriorities         (GRID *theGrid);
void    SetGhostObjectPriorities    (GRID *theGrid);

/* from trans.c */
int             TransferGrid                            (MULTIGRID *theMG);
int             TransferGridFromLevel           (MULTIGRID *theMG, INT level);
void    AMGAgglomerate                          (MULTIGRID *theMG);

/* from identify.c */
void    IdentifyInit                                    (MULTIGRID *theMG);
void    IdentifyExit                                    (void);
INT             Identify_Objects_of_ElementSide (GRID *theGrid, ELEMENT *theElement, INT i);
INT             Identify_SonNodesAndSonEdges    (GRID *theGrid);

/* from overlap.c */

INT             UpdateGridOverlap                       (GRID *theGrid);
INT             ConnectGridOverlap                      (GRID *theGrid);
INT             ConnectVerticalOverlap (MULTIGRID *theMG);

/* from priority.c */
void    SetGhostObjectPriorities        (GRID *theGrid);
INT             SetBorderPriorities                     (GRID *theGrid);
INT             SetGridBorderPriorities         (GRID *theGrid);

/* from partition.c */
INT             CheckPartitioning                       (MULTIGRID *theMG);
INT             RestrictPartitioning            (MULTIGRID *theMG);

/* from pgmcheck.c */
INT             CheckInterfaces                         (GRID *theGrid);

/*
 * COMPATABILITY FUNCTIONS FOR OLDER dune-grid RELEASES
 */

/**
 * get global DDD context.
 * This only exists for compatability with old versions of dune-grid.
 */
DDD::DDDContext& globalDDDContext();

/**
 * set global DDD context.
 * This only exists for compatability with old versions of dune-grid.
 */
void globalDDDContext(const std::shared_ptr<DDD::DDDContext>& context);

/**
 * invalidate global DDD context.
 * This only exists for compatability with old versions of dune-grid.
 */
void globalDDDContext(std::nullptr_t);

using ComProcPtr = int (*)(DDD_OBJ, void *);
void DDD_IFOneway(DDD_IF, DDD_IF_DIR, size_t, ComProcPtr, ComProcPtr);
int* DDD_InfoProcList(DDD_HDR);

#endif /* ModelP */

END_UGDIM_NAMESPACE

#endif /* __PARALLEL_H__ */
