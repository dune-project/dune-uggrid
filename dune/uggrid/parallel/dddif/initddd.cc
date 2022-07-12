// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*																			*/
/* File:	  initddd.c														*/
/*																			*/
/* Purpose:   register ug structs for handling them by ddd					*/
/*																			*/
/* Author:	  Stefan Lang, Klaus Birken										*/
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*																			*/
/* History:   09.05.95 begin, ugp version 3.0								*/
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
#include <cstddef>
#include <cstdlib>

#include "parallel.h"
#include <dune/uggrid/low/debug.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/gm/ugm.h>      /* for GetFreeOBJT() */
#include <dune/uggrid/parallel/ddd/include/memmgr.h>
#include <dune/uggrid/ugdevices.h>

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


/* macro for easier definition of DDD_TYPEs */
#define ELDEF(type,member)   offsetof(type,member), sizeof(type :: member)

/* macro for easy definition of type mapping UG<->DDD */
#define MAP_TYPES(ugt,dddt)   { int _ugt=(ugt); \
                                ddd_ctrl(context).ugtypes[(dddt)] = _ugt;     \
                                ddd_ctrl(context).types[_ugt] = (dddt);       \
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

/* DDD interfaces needed for distributed computation */
DDD_IF NS_DIM_PREFIX ElementIF, NS_DIM_PREFIX ElementSymmIF, NS_DIM_PREFIX ElementVIF, NS_DIM_PREFIX ElementSymmVIF,
NS_DIM_PREFIX ElementVHIF, NS_DIM_PREFIX ElementSymmVHIF;
DDD_IF NS_DIM_PREFIX BorderNodeIF, NS_DIM_PREFIX BorderNodeSymmIF, NS_DIM_PREFIX OuterNodeIF, NS_DIM_PREFIX NodeVIF,
NS_DIM_PREFIX NodeIF, NS_DIM_PREFIX NodeAllIF;
DDD_IF NS_DIM_PREFIX BorderVectorIF, NS_DIM_PREFIX BorderVectorSymmIF,
NS_DIM_PREFIX OuterVectorIF, NS_DIM_PREFIX OuterVectorSymmIF,
NS_DIM_PREFIX VectorVIF, NS_DIM_PREFIX VectorVAllIF, NS_DIM_PREFIX VectorIF;
/* DDD interfaces for node communication */
DDD_IF NS_DIM_PREFIX Node_InteriorBorder_All_IF;
/* DDD interfaces for facet (side vector) communication */
DDD_IF NS_DIM_PREFIX Facet_InteriorBorder_All_IF;

/* DDD interfaces for edge communication */
DDD_IF NS_DIM_PREFIX EdgeIF, NS_DIM_PREFIX BorderEdgeSymmIF, NS_DIM_PREFIX EdgeHIF, NS_DIM_PREFIX EdgeVHIF,
NS_DIM_PREFIX EdgeSymmVHIF;


/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

enum ElemTypeFlag { Inside, Boundary };


/****************************************************************************/
/*
   void ddd_InitGenericElement -

   SYNOPSIS:
   static void ddd_InitGenericElement (INT tag, DDD_TYPE dddType, int etype);

   PARAMETERS:
   .  tag
   .  dddType
   .  etype

   DESCRIPTION:

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void ddd_InitGenericElement(DDD::DDDContext& context, INT tag, DDD_TYPE dddType, int etype)
{
  auto& dddctrl = ddd_ctrl(context);
  GENERAL_ELEMENT                 *desc = element_descriptors[tag];
  UINT gbits = 0;

  size_t ps  = sizeof(void *);
  size_t r = offsetof(generic_element,refs);

  /* compute global fields of control word entry */
  gbits = ~(((1<<NSONS_LEN)-1)<<NSONS_SHIFT);
  PRINTDEBUG(dddif,1,("ddd_InitGenericElement(): gbits=%08x size=%d\n",
                      gbits,sizeof(generic_element::control)));

  /* initialize base part (valid for all elements) */
  DDD_TypeDefine(context, dddType,
                 EL_DDDHDR, offsetof(generic_element,ddd),
                 /* TODO: delete this					*/
                 /*		EL_GDATA,  ELDEF(generic_element,control),	*/
                 EL_GBITS,  ELDEF(generic_element,control), &gbits,

                 /* TODO: id muss umgerechnet werden! (?) */
                 EL_GDATA,  ELDEF(generic_element,id),
                 EL_GDATA,  ELDEF(generic_element,flag),
                 EL_GDATA,  ELDEF(generic_element,property),

                 /* LDATA, because Dune indices are local */
                 EL_LDATA,  ELDEF(generic_element,levelIndex),
                 EL_LDATA,  ELDEF(generic_element,leafIndex),

                 EL_GDATA,  ELDEF(generic_element,lb1),
                 EL_LDATA,  ELDEF(generic_element,pred),
                 EL_LDATA,  ELDEF(generic_element,succ),
                 EL_CONTINUE);


  /* initialize generic part */
  /* NOTE: references to 'union element' are denoted by the ref-type
     dddType (i.e., the DDD_TYPE of the currently defined element itself).
     TODO: this should be replaced by a more explicit TypeGenericElement in
     later versions (code would be more readable). */

  DDD_TypeDefine(context, dddType,
                 EL_OBJPTR, r+n_offset[tag]*sizeof(void*),       ps*desc->corners_of_elem, dddctrl.TypeNode,
                 EL_OBJPTR, r+father_offset[tag]*sizeof(void*),  ps,                       dddType,
                 /* TODO: delete
                    #ifdef UG_DIM_2
                                 EL_LDATA, r+sons_offset[tag]*sizeof(void*),    ps*desc->max_sons_of_elem,
                    #endif
                    #ifdef UG_DIM_3
                                 EL_LDATA, r+sons_offset[tag]*sizeof(void*),    ps*1,
                    #endif
                  */
                 EL_LDATA, r+sons_offset[tag]*sizeof(void*),     ps*2,
                 EL_OBJPTR, r+nb_offset[tag]*sizeof(void*),      ps*desc->sides_of_elem,   dddType,
                 EL_CONTINUE);


  /* optional components */

  if (ddd_ctrl(context).elemData)
    DDD_TypeDefine(context, dddType,
                   EL_OBJPTR, r+evector_offset[tag]*sizeof(void*), ps*1,     dddctrl.TypeVector,
                   EL_CONTINUE);

        #ifdef UG_DIM_3
  if (ddd_ctrl(context).sideData)
    DDD_TypeDefine(context, dddType,
                   EL_OBJPTR, r+svector_offset[tag]*sizeof(void*), ps*desc->sides_of_elem, dddctrl.TypeVector,
                   EL_CONTINUE);
        #endif

  if (etype==Inside)
  {
    DDD_TypeDefine(context, dddType, EL_END, static_cast<size_t>(desc->inner_size));

    /* init type mapping arrays */
    MAP_TYPES(MAPPED_INNER_OBJT_TAG(tag), dddType);
    ddd_ctrl(context).dddObj[MAPPED_INNER_OBJT_TAG(tag)] = true;
  }
  else
  {
    DDD_TypeDefine(context, dddType,
                   EL_LDATA, r+side_offset[tag]*sizeof(void*),  ps*desc->sides_of_elem,
                   EL_END, static_cast<size_t>(desc->bnd_size));

    /* init type mapping arrays */
    MAP_TYPES(MAPPED_BND_OBJT_TAG(tag), dddType);
    ddd_ctrl(context).dddObj[MAPPED_BND_OBJT_TAG(tag)] = true;
  }

  /* set mergemode to maximum */
  DDD_PrioMergeDefault(context, dddType, PRIOMERGE_MAXIMUM);
  /* TODO: set prios
          DDD_PrioMergeDefine(context, dddType, PrioHGhost, PrioVGhost, PrioVHGhost);
          DDD_PrioMergeDefine(context, dddType, PrioHGhost, PrioVHGhost, PrioVHGhost);
          DDD_PrioMergeDefine(context, dddType, PrioVGhost, PrioVHGhost, PrioVHGhost);
          DDD_PrioMergeDefine(context, dddType, PrioHGhost, PrioMaster, PrioMaster);
          DDD_PrioMergeDefine(context, dddType, PrioVGhost, PrioMaster, PrioMaster);
          DDD_PrioMergeDefine(context, dddType, PrioVHGhost, PrioMaster, PrioMaster);
   */

}


/****************************************************************************/
/*																			*/
/* Function:  ddd_DeclareTypes												*/
/*																			*/
/* Purpose:   declare ug data structures as DDD_TYPES						*/
/*																			*/
/* Input:     void                                                              */
/*																			*/
/* Output:    void                                                                                                              */
/*																			*/
/*																			*/
/****************************************************************************/


/****************************************************************************/
/*
   void ddd_DeclareTypes - declare ug data structures as DDD_TYPES

   SYNOPSIS:
   static void ddd_DeclareTypes (void);

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function declares ug data structures as DDD_TYPES

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void ddd_DeclareTypes(DDD::DDDContext& context)
{
  /* NOTE: (960410 KB)

          - handling of Vector and Matrix types may be
            wrong, types array entries might be used differently
            by ug (e.g., during allocation/deallocation). TODO ueberpruefen!
            (beziehung algebra.c <-> ugm.c)

          - UGTypes for elements will be computed later. therefore the
            initialization of types[] entries for elements
            is done (after the UGTypes are known) during ddd_InitGenericElement.
            TODO remove this clumsy exception! how?

          - variables TypeXXXX containing the proper DDD_TYPEs may be
            superfluous. It would be possible to replace all occurrences
            by macros like DDDType(VEOBJ), which would be implemented as
     #define DDDType(ugtype)  (ddd_ctrl(context).types[ugtype])
            TODO check this!
            pros: no double information. currently, TypeXXX may differ
                  from corresponding ddd_ctrl(context).types[] entry.
            cons: will this be compatible with alloc/dealloc and TypeDefine
                  of ug-general-elements?
   */


  auto& dddctrl = ddd_ctrl(context);

  /* 1. DDD objects (with DDD_HEADER) */

  dddctrl.TypeVector = DDD_TypeDeclare(context, "Vector");
  MAP_TYPES(VEOBJ, dddctrl.TypeVector);
  dddctrl.dddObj[VEOBJ] = true;

  dddctrl.TypeIVertex = DDD_TypeDeclare(context, "IVertex");
  MAP_TYPES(IVOBJ, dddctrl.TypeIVertex);
  dddctrl.dddObj[IVOBJ] = true;

  dddctrl.TypeBVertex = DDD_TypeDeclare(context, "BVertex");
  MAP_TYPES(BVOBJ, dddctrl.TypeBVertex);
  dddctrl.dddObj[BVOBJ] = true;

  dddctrl.TypeNode = DDD_TypeDeclare(context, "Node");
  MAP_TYPES(NDOBJ, dddctrl.TypeNode);
  dddctrl.dddObj[NDOBJ] = true;

        #ifdef UG_DIM_2
  dddctrl.TypeTrElem  = DDD_TypeDeclare(context, "TrElem");
  dddctrl.TypeTrBElem = DDD_TypeDeclare(context, "TrBElem");
  dddctrl.TypeQuElem  = DDD_TypeDeclare(context, "QuElem");
  dddctrl.TypeQuBElem = DDD_TypeDeclare(context, "QuBElem");
        #endif /* TWODIM */

        #ifdef UG_DIM_3
  dddctrl.TypeTeElem  = DDD_TypeDeclare(context, "TeElem");
  dddctrl.TypeTeBElem = DDD_TypeDeclare(context, "TeBElem");
  dddctrl.TypePyElem  = DDD_TypeDeclare(context, "PyElem");
  dddctrl.TypePyBElem = DDD_TypeDeclare(context, "PyBElem");
  dddctrl.TypePrElem  = DDD_TypeDeclare(context, "PrElem");
  dddctrl.TypePrBElem = DDD_TypeDeclare(context, "PrBElem");
  dddctrl.TypeHeElem  = DDD_TypeDeclare(context, "HeElem");
  dddctrl.TypeHeBElem = DDD_TypeDeclare(context, "HeBElem");
        #endif /* THREEDIM */

  /* edge type not unique:                    */
  /* edge is DDD object for 2D and 3D         */
  /* edge is DDD data object for 2D           */
  dddctrl.TypeEdge = DDD_TypeDeclare(context, "Edge");
  MAP_TYPES(EDOBJ, dddctrl.TypeEdge);
  dddctrl.dddObj[EDOBJ] = true;

  /* 2. DDD data objects (without DDD_HEADER) */

  dddctrl.TypeBndP = DDD_TypeDeclare(context, "BndP");
  static const INT objtBndP = GetFreeOBJT();
  MAP_TYPES(objtBndP, dddctrl.TypeBndP);

  dddctrl.TypeBndS = DDD_TypeDeclare(context, "BndS");
  static const INT objtBndS = GetFreeOBJT();
  MAP_TYPES(objtBndS, dddctrl.TypeBndS);
}


/****************************************************************************/
/*
   ddd_DefineTypes - define previously declared DDD_TYPES

   SYNOPSIS:
   static void ddd_DefineTypes (void);

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function defines previously declared DDD_TYPES
   Note: this function depends on previous definition of all necessary ug-generic-elements.

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void ddd_DefineTypes(DDD::DDDContext& context)
{
  auto& dddctrl = ddd_ctrl(context);
  INT size;
  UINT gbits = 0;

  /* 1. DDD objects (with DDD_HEADER) */

  /*
   * A side vector's `VECTORSIDE` depends on which of the `objects` is used
   * as a representative. To ensure consistency, both are handled in the same
   * place (`ElementObjMkCons`).
   */
  gbits = ~(((1 << VECTORSIDE_LEN)-1) << VECTORSIDE_SHIFT);
  DDD_TypeDefine(context, dddctrl.TypeVector,
                 EL_DDDHDR, offsetof(VECTOR,ddd),
                 EL_GBITS,  ELDEF(VECTOR,control), &gbits,

                 /* object must be LDATA, because reftype may be a non-DDD-object */
                 /* (e.g., edge). therefore, 'object' must be updated by MKCONS-  */
                 /* handler of associated object. 960404 KB */
                 /* TODO: decide whether LDATA or OBJPTR for different VectorTypes*/
                 /* EL_OBJPTR, ELDEF(VECTOR,object), TypeNode, */
                 EL_LDATA,  ELDEF(VECTOR,object),
                 EL_LDATA,  ELDEF(VECTOR,pred),
                 EL_LDATA,  ELDEF(VECTOR,succ),
                 EL_GDATA,  ELDEF(VECTOR,index),
                 EL_GDATA,  ELDEF(VECTOR,leafIndex),
                 EL_LDATA,  ELDEF(VECTOR,start),

                 /* TODO: value wird noch ausgelassen. Feld variabler Laenge? */
                 /* bei Entscheidung 'value': kein weiteres Feld
                         bei ent. 'userdata *': EL_GDATA-feld        */
                 EL_GDATA,  ELDEF(VECTOR,value),
                 EL_END,    sizeof(VECTOR)
                 );

  /* set mergemode to maximum */
  DDD_PrioMergeDefault(context, dddctrl.TypeVector, PRIOMERGE_MAXIMUM);

  /* compute global fields it control word entry */
  gbits = ~((((1<<ONEDGE_LEN)-1)<<ONEDGE_SHIFT) |
            (((1<<NOOFNODE_LEN)-1)<<NOOFNODE_SHIFT));
  PRINTDEBUG(dddif,1,("ddd_DefineTypes(): TypeI/BVertex gbits=%08x size=%d\n",
                      gbits,sizeof(ivertex::control)));

  DDD_TypeDefine(context, dddctrl.TypeIVertex,
                 EL_DDDHDR, offsetof(ivertex,ddd),
                 /* TODO: delete
                                 EL_GDATA,  ELDEF(ivertex,control),
                  */
                 EL_GBITS,  ELDEF(ivertex,control), &gbits,

                 /* TODO: muss umgerechnet werden! */
                 EL_GDATA,  ELDEF(ivertex,id),
                 EL_GDATA,  ELDEF(ivertex,x),
                 EL_GDATA,  ELDEF(ivertex,xi),
                 EL_LDATA,  ELDEF(ivertex,leafIndex),
                 EL_LDATA,  ELDEF(ivertex,pred),
                 EL_LDATA,  ELDEF(ivertex,succ),
                 EL_LDATA,  ELDEF(ivertex,data),

                 /* TODO Does father have to be LDATA or OBJPTR?     */
                 /* LDATA, father is valid only locally,             */
                 /* and depends on vertical load balancing.          */
                #ifdef UG_DIM_2
                 /* TODO: ref-typ muss eigentlich {TypeTrElem,TypeTrBElem} sein! */
                 EL_OBJPTR, ELDEF(ivertex,father), dddctrl.TypeTrElem,
                #endif
                #ifdef UG_DIM_3
                 EL_LDATA,  ELDEF(ivertex,father),
                #endif

                #ifdef TOPNODE
                 /* TODO topnode wirklich OBJPTR? */
                 EL_LDATA,  ELDEF(ivertex,topnode),
                #endif
                 EL_END,    sizeof(ivertex)
                 );

  /* set mergemode to maximum */
  DDD_PrioMergeDefault(context, dddctrl.TypeIVertex, PRIOMERGE_MAXIMUM);


  DDD_TypeDefine(context, dddctrl.TypeBVertex,
                 EL_DDDHDR, offsetof(bvertex,ddd),
                 /* TODO: delete
                                 EL_GDATA,  ELDEF(bvertex,control),
                  */
                 EL_GBITS,  ELDEF(bvertex,control), &gbits,

                 /* TODO: muss umgerechnet werden! Nooeee! */
                 EL_GDATA,  ELDEF(bvertex,id),
                 EL_GDATA,  ELDEF(bvertex,x),
                 EL_GDATA,  ELDEF(bvertex,xi),
                 EL_LDATA,  ELDEF(bvertex,leafIndex),
                 EL_LDATA,  ELDEF(bvertex,pred),
                 EL_LDATA,  ELDEF(bvertex,succ),
                 EL_LDATA,  ELDEF(bvertex,data),

                 /* TODO Does father have to be LDATA or OBJPTR?     */
                 /* LDATA, father is valid only locally,             */
                 /* and depends on vertical load balancing.          */
                #ifdef UG_DIM_2
                 /* TODO: ref-typ muss eigentlich {TypeTrElem,TypeTrBElem} sein! */
                 EL_OBJPTR, ELDEF(bvertex,father), dddctrl.TypeTrElem,
                #endif
                #ifdef UG_DIM_3
                 EL_LDATA,  ELDEF(bvertex,father),
                #endif

                #ifdef TOPNODE
                 /* TODO topnode wirklich OBJPTR?, Nooeee! */
                 EL_LDATA,  ELDEF(bvertex,topnode),
                #endif
                 EL_LDATA,  ELDEF(bvertex,bndp),     /* different from IVertex */
                 EL_END,    sizeof(bvertex)
                 );

  /* set mergemode to maximum */
  DDD_PrioMergeDefault(context, dddctrl.TypeBVertex, PRIOMERGE_MAXIMUM);


  DDD_TypeDefine(context, dddctrl.TypeNode,
                 EL_DDDHDR, offsetof(NODE,ddd),
                 EL_GDATA,  ELDEF(NODE,control),

                 /* TODO: muss umgerechnet werden! */
                 EL_GDATA,  ELDEF(NODE,id),
                 EL_LDATA,  ELDEF(NODE,levelIndex),
                 EL_GDATA,  ELDEF(NODE,isLeaf),
                 EL_LDATA,  ELDEF(NODE,pred),
                 EL_LDATA,  ELDEF(NODE,succ),

                 /* TODO What is start? */
                 EL_LDATA,  ELDEF(NODE,start),

                 /* father may be one of node or edge */
                 EL_OBJPTR, ELDEF(NODE,father),   DDD_TYPE_BY_HANDLER, NFatherObjType,

                 EL_OBJPTR, ELDEF(NODE,son),      dddctrl.TypeNode,

                 /* TODO: ref-typ muss eigentlich {TypeIVertex,TypeBVertex} sein! */
                 EL_OBJPTR, ELDEF(NODE,myvertex), dddctrl.TypeIVertex,
                 EL_CONTINUE);

  DDD_TypeDefine(context, dddctrl.TypeNode,
                 EL_END, sizeof(NODE));

  /* set mergemode to maximum */
  DDD_PrioMergeDefault(context, dddctrl.TypeNode, PRIOMERGE_MAXIMUM);


        #ifdef UG_DIM_2
  ddd_InitGenericElement(context, TRIANGLE,      dddctrl.TypeTrElem,  Inside);
  ddd_InitGenericElement(context, TRIANGLE,      dddctrl.TypeTrBElem, Boundary);
  ddd_InitGenericElement(context, QUADRILATERAL, dddctrl.TypeQuElem,  Inside);
  ddd_InitGenericElement(context, QUADRILATERAL, dddctrl.TypeQuBElem, Boundary);
        #endif /* TWODIM */

        #ifdef UG_DIM_3
  ddd_InitGenericElement(context, TETRAHEDRON, dddctrl.TypeTeElem,  Inside);
  ddd_InitGenericElement(context, TETRAHEDRON, dddctrl.TypeTeBElem, Boundary);
  ddd_InitGenericElement(context, PYRAMID,     dddctrl.TypePyElem,  Inside);
  ddd_InitGenericElement(context, PYRAMID,     dddctrl.TypePyBElem, Boundary);
  ddd_InitGenericElement(context, PRISM,       dddctrl.TypePrElem,  Inside);
  ddd_InitGenericElement(context, PRISM,       dddctrl.TypePrBElem, Boundary);
  ddd_InitGenericElement(context, HEXAHEDRON,  dddctrl.TypeHeElem,  Inside);
  ddd_InitGenericElement(context, HEXAHEDRON,  dddctrl.TypeHeBElem, Boundary);
        #endif /* THREEDIM */

  /* 2. DDD data objects (without DDD_HEADER) */

  /* compute global fields it control word entry */
  gbits = ~(((1<<NO_OF_ELEM_LEN)-1)<<NO_OF_ELEM_SHIFT);
  PRINTDEBUG(dddif,1,("ddd_DefineTypes(): TypeEdge gbits=%08x size=%d\n",
                      gbits,sizeof(link::control)));

  DDD_TypeDefine(context, dddctrl.TypeEdge,
                 /* link 0 data */
                 /*TODO: now unique
                    #ifdef UG_DIM_2
                                 EL_GDATA,  ELDEF(EDGE,links[0].control),
                    #endif
                    #ifdef UG_DIM_3
                                 EL_LDATA,  ELDEF(EDGE,links[0].control),
                    #endif
                  */
                 EL_GBITS,  ELDEF(EDGE,links[0].control), &gbits,
                 EL_LDATA,  ELDEF(EDGE,links[0].next),
                 EL_OBJPTR, ELDEF(EDGE,links[0].nbnode), dddctrl.TypeNode,

                 /* link 1 data */
                 EL_GDATA,  ELDEF(EDGE,links[1].control),
                 EL_LDATA,  ELDEF(EDGE,links[1].next),
                 EL_OBJPTR, ELDEF(EDGE,links[1].nbnode), dddctrl.TypeNode,

                 EL_LDATA,  ELDEF(EDGE,levelIndex),
                 EL_LDATA,  ELDEF(EDGE,leafIndex),
                 EL_GDATA,  ELDEF(EDGE,id),
                 EL_DDDHDR, offsetof(EDGE,ddd),

                 EL_OBJPTR, ELDEF(EDGE,midnode),  dddctrl.TypeNode,
                 EL_CONTINUE);

  if (dddctrl.edgeData)
    DDD_TypeDefine(context, dddctrl.TypeEdge,
                   EL_OBJPTR, ELDEF(EDGE,vector), dddctrl.TypeVector,
                   EL_CONTINUE);

  /* See the corresponding line for TypeNode for an explanation of why
   * the object size is modified here. */
  size = sizeof(EDGE) - ((ddd_ctrl(context).edgeData) ? 0 : sizeof(VECTOR*));
  DDD_TypeDefine(context, dddctrl.TypeEdge, EL_END, size);

  /* set mergemode to maximum */
  DDD_PrioMergeDefault(context, dddctrl.TypeEdge, PRIOMERGE_MAXIMUM);

}


/****************************************************************************/
/*
   ddd_IfInit - define the communication interfaces needed in ug for management by DDD

   SYNOPSIS:
   static void ddd_IfInit(DDD::DDDContext& context);

   PARAMETERS:
   .  context

   DESCRIPTION:
   This function defines the communication interfaces needed in ug for management by DDD

   RETURN VALUE:
   void

   \note Not all of these interfaces are actually needed by Dune.
     Those can be removed.
 */
/****************************************************************************/

static void ddd_IfInit(DDD::DDDContext& context)
{
  auto& dddctrl = ddd_ctrl(context);

  DDD_TYPE O[8];
  int nO;
  DDD_PRIO A[8];
  DDD_PRIO B[8];


  /* define element interfaces */
#ifdef UG_DIM_2
  O[0] = dddctrl.TypeTrElem; O[1] = dddctrl.TypeTrBElem;
  O[2] = dddctrl.TypeQuElem; O[3] = dddctrl.TypeQuBElem;
  nO = 4;
#endif

#ifdef UG_DIM_3
  O[0] = dddctrl.TypeTeElem; O[1] = dddctrl.TypeTeBElem;
  O[2] = dddctrl.TypePyElem; O[3] = dddctrl.TypePyBElem;
  O[4] = dddctrl.TypePrElem; O[5] = dddctrl.TypePrBElem;
  O[6] = dddctrl.TypeHeElem; O[7] = dddctrl.TypeHeBElem;
  nO = 8;
#endif

  A[0] = PrioMaster;
  B[0] = PrioHGhost; B[1] = PrioVHGhost;
  dddctrl.ElementIF = DDD_IFDefine(context, nO,O,1,A,2,B);
  DDD_IFSetName(context, dddctrl.ElementIF, "ElementIF: Master->HGhost/VHGhost");

  A[0] = PrioMaster; A[1] = PrioHGhost; A[2] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioHGhost; B[2] = PrioVHGhost;
  dddctrl.ElementSymmIF = DDD_IFDefine(context, nO,O,3,A,3,B);
  DDD_IFSetName(context, dddctrl.ElementSymmIF, "ElementSymmIF: Master/HGhost/VHGhost");

  A[0] = PrioMaster;
  B[0] = PrioVGhost; B[1] = PrioVHGhost;
  dddctrl.ElementVIF = DDD_IFDefine(context, nO,O,1,A,2,B);
  DDD_IFSetName(context, dddctrl.ElementVIF, "ElementVIF: Master->VGhost/VHGhost");

  A[0] = PrioMaster; A[1] = PrioVGhost; A[2] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioVGhost; B[2] = PrioVHGhost;
  dddctrl.ElementSymmVIF = DDD_IFDefine(context, nO,O,3,A,3,B);
  DDD_IFSetName(context, dddctrl.ElementSymmVIF, "ElementSymmVIF: Master/VGhost/VHGhost");

  A[0] = PrioMaster;
  B[0] = PrioVGhost; B[1] = PrioHGhost; B[2] = PrioVHGhost;
  dddctrl.ElementVHIF = DDD_IFDefine(context, nO,O,1,A,3,B);
  DDD_IFSetName(context, dddctrl.ElementVHIF, "ElementVHIF: Master->VGhost/HGhost/VHGhost");

  A[0] = PrioMaster; A[1] = PrioVGhost; A[2] = PrioHGhost; A[3] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioVGhost; B[2] = PrioHGhost; B[3] = PrioVHGhost;
  dddctrl.ElementSymmVHIF = DDD_IFDefine(context, nO,O,4,A,4,B);
  DDD_IFSetName(context, dddctrl.ElementSymmVHIF, "ElementSymmVHIF: Master/VGhost/HGhost/VHGhost");


  /* define node interfaces */
  O[0] = dddctrl.TypeNode;

  A[0] = PrioBorder;
  B[0] = PrioMaster;
  dddctrl.BorderNodeIF = DDD_IFDefine(context, 1,O,1,A,1,B);
  DDD_IFSetName(context, dddctrl.BorderNodeIF, "BorderNodeIF: Border->Master");

  A[0] = PrioMaster; A[1] = PrioBorder;
  B[0] = PrioMaster; B[1] = PrioBorder;
  dddctrl.BorderNodeSymmIF = DDD_IFDefine(context, 1,O,2,A,2,B);
  DDD_IFSetName(context, dddctrl.BorderNodeSymmIF, "BorderNodeSymmIF: Border/Master");

  A[0] = PrioMaster;
  B[0] = PrioHGhost; B[1] = PrioVHGhost;
  dddctrl.OuterNodeIF = DDD_IFDefine(context, 1,O,1,A,2,B);
  DDD_IFSetName(context, dddctrl.OuterNodeIF, "OuterNodeIF: Master->HGhost/VGhost");

  A[0] = PrioMaster;
  B[0] = PrioVGhost; B[1] = PrioVHGhost;
  dddctrl.NodeVIF = DDD_IFDefine(context, 1,O,1,A,2,B);
  DDD_IFSetName(context, dddctrl.NodeVIF, "NodeVIF: Master->VGhost/VHGhost");

  A[0] = PrioMaster;
  B[0] = PrioVGhost; B[1] = PrioHGhost; B[2] = PrioVHGhost;
  dddctrl.NodeIF = DDD_IFDefine(context, 1,O,1,A,3,B);
  DDD_IFSetName(context, dddctrl.NodeIF, "NodeIF: Master->VGhost/HGhost/VHGhost");

  A[0] = PrioMaster; A[1] = PrioBorder; A[2] = PrioVGhost; A[3] = PrioHGhost; A[4] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioVGhost; B[3] = PrioHGhost; B[4] = PrioVHGhost;
  dddctrl.NodeAllIF = DDD_IFDefine(context, 1,O,5,A,5,B);
  DDD_IFSetName(context, dddctrl.NodeAllIF, "NodeAllIF: All/All");

  // The Dune InteriorBorder_All_Interface for nodes
  A[0] = PrioMaster; A[1] = PrioBorder;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioVGhost; B[3] = PrioHGhost; B[4] = PrioVHGhost;
  dddctrl.Node_InteriorBorder_All_IF = DDD_IFDefine(context, 1,O,2,A,5,B);
  DDD_IFSetName(context, dddctrl.Node_InteriorBorder_All_IF, "Node_InteriorBorder_All_IF: Master/Border->Master/Border/VGhost/HGhost/VHGhost");

  /* define vector interfaces */
  O[0] = dddctrl.TypeVector;

  A[0] = PrioBorder;
  B[0] = PrioMaster;
  dddctrl.BorderVectorIF = DDD_IFDefine(context, 1,O,1,A,1,B);
  DDD_IFSetName(context, dddctrl.BorderVectorIF, "BorderVectorIF: Border->Master");

  A[0] = PrioMaster; A[1] = PrioBorder;
  B[0] = PrioMaster; B[1] = PrioBorder;
  dddctrl.BorderVectorSymmIF = DDD_IFDefine(context, 1,O,2,A,2,B);
  DDD_IFSetName(context, dddctrl.BorderVectorSymmIF, "BorderVectorSymmIF: Master/Border");

  A[0] = PrioMaster;
  B[0] = PrioHGhost; B[1] = PrioVHGhost;
  dddctrl.OuterVectorIF = DDD_IFDefine(context, 1,O,1,A,2,B);
  DDD_IFSetName(context, dddctrl.OuterVectorIF, "OuterVectorIF: Master->HGhost/VHGhost");

  A[0] = PrioMaster; A[1] = PrioBorder; A[2] = PrioHGhost; A[3] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioHGhost; B[3] = PrioVHGhost;
  dddctrl.OuterVectorSymmIF = DDD_IFDefine(context, 1,O,4,A,4,B);
  DDD_IFSetName(context, dddctrl.OuterVectorSymmIF, "OuterVectorSymmIF: Master/Border/HGhost/VHGhost");

  A[0] = PrioMaster;
  B[0] = PrioVGhost; B[1] = PrioVHGhost;
  dddctrl.VectorVIF = DDD_IFDefine(context, 1,O,1,A,2,B);
  DDD_IFSetName(context, dddctrl.VectorVIF, "VectorVIF: Master->VGhost/VHGhost");

  A[0] = PrioMaster; A[1] = PrioBorder; A[2] = PrioVGhost; A[3] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioBorder;
  dddctrl.VectorVAllIF = DDD_IFDefine(context, 1,O,4,A,2,B);
  DDD_IFSetName(context, dddctrl.VectorVAllIF, "VectorVAllIF: Master/Border/VGhost/VHGhost->Master/Border");

  A[0] = PrioMaster;
  B[0] = PrioVGhost; B[1] = PrioVHGhost; B[2] = PrioHGhost;
  dddctrl.VectorIF = DDD_IFDefine(context, 1,O,1,A,3,B);
  DDD_IFSetName(context, dddctrl.VectorIF, "VectorIF: Master->VGhost/VHGhost/HGhost");

  // The Dune InteriorBorder_All_Interface for facets
  A[0] = PrioMaster; A[1] = PrioBorder;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioVGhost; B[3] = PrioHGhost; B[4] = PrioVHGhost;
  dddctrl.Facet_InteriorBorder_All_IF = DDD_IFDefine(context, 1,O,2,A,5,B);
  DDD_IFSetName(context, dddctrl.Facet_InteriorBorder_All_IF, "Facet_InteriorBorder_All_IF: Master/Border->Master/Border/VGhost/HGhost/VHGhost");

  // The Dune All_All_Interface for facets
  A[0] = PrioMaster; A[1] = PrioBorder; A[2] = PrioVGhost; A[3] = PrioHGhost; A[4] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioVGhost; B[3] = PrioHGhost; B[4] = PrioVHGhost;
  dddctrl.Facet_All_All_IF = DDD_IFDefine(context, 1,O,5,A,5,B);
  DDD_IFSetName(context, dddctrl.Facet_All_All_IF, "Facet_All_All_IF: Master/Border/VGhost/HGhost/VHGhost->Master/Border/VGhost/HGhost/VHGhost");

  /* define vertex interfaces */
  O[0] = dddctrl.TypeIVertex; O[1] = dddctrl.TypeBVertex;

  A[0] = PrioMaster;
  B[0] = PrioMaster;
  dddctrl.VertexIF = DDD_IFDefine(context, 2,O,1,A,1,B);
  DDD_IFSetName(context, dddctrl.VertexIF, "VertexIF: Master<->Master");


  /* define edge interfaces */
  O[0] = dddctrl.TypeEdge;

  A[0] = PrioMaster;
  B[0] = PrioMaster;
  dddctrl.EdgeIF = DDD_IFDefine(context, 1,O,1,A,1,B);
  DDD_IFSetName(context, dddctrl.EdgeIF, "EdgeIF: Master<->Master");

  A[0] = PrioMaster; A[1] = PrioBorder;
  B[0] = PrioMaster; B[1] = PrioBorder;
  dddctrl.BorderEdgeSymmIF = DDD_IFDefine(context, 1,O,2,A,2,B);
  DDD_IFSetName(context, dddctrl.BorderEdgeSymmIF, "BorderEdgeSymmIF: Master/Border");

  A[0] = PrioMaster; A[1] = PrioBorder;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioHGhost; B[3] = PrioVHGhost;
  dddctrl.EdgeHIF = DDD_IFDefine(context, 1,O,2,A,4,B);
  DDD_IFSetName(context, dddctrl.EdgeHIF, "EdgeHIF: Master/Border->Master/Border/PrioHGhost/PrioVHGhost");

  A[0] = PrioMaster; A[1] = PrioBorder;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioVGhost; B[3] = PrioHGhost; B[4] = PrioVHGhost;
  dddctrl.EdgeVHIF = DDD_IFDefine(context, 1,O,2,A,5,B);
  DDD_IFSetName(context, dddctrl.EdgeVHIF, "EdgeVHIF: Master/Border->Master/Border/VGhost/HGhost/VHGhost");

  A[0] = PrioMaster; A[1] = PrioBorder; A[2] = PrioVGhost; A[3] = PrioHGhost; A[4] = PrioVHGhost;
  B[0] = PrioMaster; B[1] = PrioBorder; B[2] = PrioVGhost; B[3] = PrioHGhost; B[4] = PrioVHGhost;
  dddctrl.EdgeSymmVHIF = DDD_IFDefine(context, 1,O,5,A,5,B);
  DDD_IFSetName(context, dddctrl.EdgeSymmVHIF, "EdgeSymmVHIF: Master/Border/VGhost/HGhost/VHGhost");

}


/****************************************************************************/
/*
   InitDDDTypes - define DDD_TYPEs

   SYNOPSIS:
   static void InitDDDTypes (DDD::DDDContext& context);

   PARAMETERS:
   .  context

   DESCRIPTION:
   This function must be called once before creation of DDD-objects. It depends on correct and complete initialization of all ug-generic-elements, therefore it must be called after completion of InitElementTypes(). As InitElementTypes() will be called whenever new Multigrids are created/opened, an execution guard prevents this function from multiple execution.

   RETURN VALUE:
   void
 */
/****************************************************************************/

static void InitDDDTypes(DDD::DDDContext& context)
{
  auto& dddctrl = ddd_ctrl(context);

  /* prevent from multiple execution */
  if (dddctrl.allTypesDefined)
    return;
  dddctrl.allTypesDefined = true;

  ddd_DefineTypes(context);


  /* display ddd types */
  IFDEBUG(dddif,1)
  DDD_TypeDisplay(context, dddctrl.TypeVector);
  DDD_TypeDisplay(context, dddctrl.TypeIVertex);
  DDD_TypeDisplay(context, dddctrl.TypeBVertex);
  DDD_TypeDisplay(context, dddctrl.TypeNode);
  DDD_TypeDisplay(context, dddctrl.TypeEdge);

        #ifdef UG_DIM_2
  DDD_TypeDisplay(context, dddctrl.TypeTrElem);
  DDD_TypeDisplay(context, dddctrl.TypeTrBElem);
  DDD_TypeDisplay(context, dddctrl.TypeQuElem);
  DDD_TypeDisplay(context, dddctrl.TypeQuBElem);
        #endif

        #ifdef UG_DIM_3
  DDD_TypeDisplay(context, dddctrl.TypeTeElem);
  DDD_TypeDisplay(context, dddctrl.TypeTeBElem);
  DDD_TypeDisplay(context, dddctrl.TypePyElem);
  DDD_TypeDisplay(context, dddctrl.TypePyBElem);
  DDD_TypeDisplay(context, dddctrl.TypePrElem);
  DDD_TypeDisplay(context, dddctrl.TypePrBElem);
  DDD_TypeDisplay(context, dddctrl.TypeHeElem);
  DDD_TypeDisplay(context, dddctrl.TypeHeBElem);
        #endif

  /* display dependent types */
        #ifdef UG_DIM_2
  DDD_TypeDisplay(context, dddctrl.TypeEdge);
        #endif
  ENDDEBUG

  ddd_HandlerInit(context, HSET_XFER);
}



/****************************************************************************/
/*
   InitCurrMG - initialize the current multigrid which is handled by DDD

   SYNOPSIS:
   void InitCurrMG (MULTIGRID *MG);

   PARAMETERS:
   .  MG

   DESCRIPTION:
   This function initializes the current multigrid which is handled by DDD

   RETURN VALUE:
   void
 */
/****************************************************************************/


void NS_DIM_PREFIX InitCurrMG (MULTIGRID *MG)
{
  auto& dddctrl = ddd_ctrl(MG->dddContext());
  dddctrl.currMG = MG;

  dddctrl.nodeData = 0;
  dddctrl.edgeData = 0;
  dddctrl.elemData = 0;
  dddctrl.sideData = VEC_DEF_IN_OBJ_OF_MG(dddctrl.currMG,SIDEVEC);

  InitDDDTypes(MG->dddContext());
}


/****************************************************************************/
/*
   CheckInitParallel - check for correct initialization of dddif subsystem

   SYNOPSIS:
   static int CheckInitParallel (void);

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function performs checks for correct initialization of dddif subsystem

   RETURN VALUE:
   int
     error value
 */
/****************************************************************************/


static int CheckInitParallel(const DDD::DDDContext& context)
{
  int i;

  /* check for valid UGTYPE for given DDD_TYPE */
  if (OBJT_MAX == MAXOBJECTS)
  {
    printf("ERROR in InitParallel: OBJT_MAX!=MAXOBJECTS\n");
    return(__LINE__);
  }

  for(i=1; i<MAXDDDTYPES && UGTYPE(context, i)>=0; i++)
  {
    /* check for valid UGTYPE for given DDD_TYPE */
    if (UGTYPE(context, i) > OBJT_MAX)
    {
      printf("ERROR in InitParallel: OBJT=%d > OBJT_MAX=%d\n",
             UGTYPE(context, i), OBJT_MAX);
      return(__LINE__);
    }

    /* check for correct mapping and re-mapping */
    if (DDDTYPE(context, UGTYPE(context, i))!=i)
    {
      printf("ERROR in InitParallel: invalid type mapping for OBJT=%d\n",
             UGTYPE(context, i));
      return(__LINE__);
    }
  }

  /* no errors */
  return(0);
}



/****************************************************************************/
/*
   InitDDD - initialize the ddd library

   SYNOPSIS:
   int InitDDD (int *argc, char ***argv);

   PARAMETERS:
   .  argc - pointer to number of arguments
   .  argv - pointer to list of argument pointers

   DESCRIPTION:
   This function initializes the ddd library by defining the ug internal issues: format of handled structs, description of handlers, definition of interfaces

   RETURN VALUE:
   int
 */
/****************************************************************************/

int NS_DIM_PREFIX InitDDD(DDD::DDDContext& context)
{
  auto& dddctrl = ddd_ctrl(context);

  INT err;
  int i;

  /* init DDD and set options */
  DDD_Init(context);

  /* we are using varsized DDD objects, turn warnings off */
  DDD_SetOption(context, OPT_WARNING_VARSIZE_OBJ, OPT_OFF);
  DDD_SetOption(context, OPT_WARNING_SMALLSIZE, OPT_OFF);

  /* no internal free list */
  DDD_SetOption(context, OPT_CPLMGR_USE_FREELIST, OPT_OFF);

  /* show messages during transfer, for debugging */
  DDD_SetOption(context, OPT_DEBUG_XFERMESGS, OPT_OFF);

  /* TODO: remove this, reference collision with Edge orientation
     in 3D */
  DDD_SetOption(context, OPT_WARNING_REF_COLLISION, OPT_OFF);

  /* treat identify tokens for one object as set */
  DDD_SetOption(context, OPT_IDENTIFY_MODE, IDMODE_SETS);

  /* dont delete objects when another copy comes in during Xfer */
  DDD_SetOption(context, OPT_XFER_PRUNE_DELETE, OPT_ON);

  /* initialize type mapping arrays */
  for(i=0; i<MAXOBJECTS; i++)
  {
    dddctrl.types[i] = -1;
    dddctrl.dddObj[i] = false;
  }
  for(i=0; i<MAXDDDTYPES; i++)
  {
    dddctrl.ugtypes[i] = -1;
  }

  /* declare DDD_TYPES, definition must be done later */
  ddd_DeclareTypes(context);
  dddctrl.allTypesDefined = false;

  DomInitParallel(dddctrl.TypeBndP,dddctrl.TypeBndS);

  ddd_IfInit(context);

  /* check for correct initialization */
  if ((err=CheckInitParallel(context))!=0)
  {
    SetHiWrd(err,__LINE__);
    return(err);
  }

  return 0;          /* no error */
}



/****************************************************************************/
/*
   ExitDDD - exit the parallel application on ddd level

   SYNOPSIS:
   int ExitDDD (void);

   PARAMETERS:
   .  void

   DESCRIPTION:
   This function exits the parallel application on ddd level

   RETURN
   N VALUE:
   void
 */
/****************************************************************************/

int NS_DIM_PREFIX ExitDDD(DDD::DDDContext& context)
{
  DDD_Exit(context);

  return 0;          /* no error */
}

#endif /* ModelP */
