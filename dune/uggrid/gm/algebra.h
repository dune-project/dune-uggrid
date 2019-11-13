// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file algebra.h
 * \ingroup gm
 */

/** \addtogroup gm
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      algebra.h                                                     */
/*                                                                          */
/* Purpose:   header for algebraic structures                               */
/*            internal interface for grid manager module                    */
/*                                                                          */
/* Author:    Klaus Johannsen                                               */
/*            Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*            Universitaet Heidelberg                                       */
/*            Im Neuenheimer Feld 294                                       */
/*            6900 Heidelberg                                               */
/*            internet: ug@ica3.uni-stuttgart.de                            */
/*                                                                          */
/*            blockvector data structure:                                   */
/*            Christian Wrobel                                              */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   1.12.93 begin, ug 3d                                          */
/*            27.09.95 blockvector implemented (Christian Wrobel)           */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __ALGEBRA__
#define __ALGEBRA__

#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

#include "gm.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*     compile time constants defining static data size (i.e. arrays)       */
/*     other constants                                                      */
/*     macros                                                               */
/*                                                                          */
/****************************************************************************/

/** @name Vector classes */
enum VectorClass {EVERY_CLASS = 0,       /*!< Class of all vectors */
                  NEWDEF_CLASS = 2,       /*!< Class of the vectors where defect needed  */
                  ACTIVE_CLASS = 3       /*!< Class of the active vectors */
};

#define GET_MATRIX(v,w,m)                                                   \
  { MATRIX *theMatrix0;                                                       \
    VECTOR *theVector0 = (v);                                                 \
    VECTOR *theVector1 = (w);                                                 \
    (m) = NULL;                                                               \
    if (theVector0 == theVector1) (m) = VSTART(theVector0);                   \
    else if (VINDEX(theVector0) > VINDEX(theVector1)) {                       \
      for (theMatrix0=MNEXT(VSTART(theVector0)); theMatrix0!=NULL;          \
           theMatrix0=MNEXT(theMatrix0))                                    \
        if (MDEST(theMatrix0)==theVector1) { (m) = theMatrix0; break; }}  \
    else { for (theMatrix0=MNEXT(VSTART(theVector1)); theMatrix0!=NULL;       \
                theMatrix0=MNEXT(theMatrix0))                                    \
             if (MDEST(theMatrix0)==theVector0) {(m) = MADJ(theMatrix0); break;}}}

/****************************************************************************/
/*                                                                          */
/* data structures exported by the corresponding source file                */
/*                                                                          */
/****************************************************************************/

extern const char *ObjTypeName[MAXVOBJECTS];

/****************************************************************************/
/*                                                                          */
/* control word definitions                                                 */
/*                                                                          */
/****************************************************************************/

enum ALGEBRA_CE {

  EBUILDCON_CE = GM_N_CE,           /* continue after gm.h entries */

  ALGEBRA_N_CE
};

/* element */
#define EBUILDCON_SHIFT                         11
#define EBUILDCON_LEN                           1
#define EBUILDCON(p)                            CW_READ(p,EBUILDCON_CE)
#define SETEBUILDCON(p,n)                       CW_WRITE(p,EBUILDCON_CE,n)

/****************************************************************************/
/*                                                                          */
/* macros for VECTORs                                                       */
/*                                                                          */
/****************************************************************************/
/** @name Macros for VECTORs  */
/*@{*/
#define VBUILDCON(p)                            VCFLAG(p)
#define SETVBUILDCON(p,n)                       SETVCFLAG(p,n)
/*@}*/
/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/** \brief Domain part for object */
INT GetDomainPart (const INT s2p[], const GEOM_OBJECT *obj, INT side);

/** @name basic create and dispose functions */
/*@{*/
INT         CreateVector                                    (GRID *theGrid, INT ObjectType, GEOM_OBJECT *object, VECTOR **vHandle);
INT         CreateSideVector                (GRID *theGrid, INT side, GEOM_OBJECT *object, VECTOR **vHandle);
INT         ReinspectSonSideVector  (GRID *g, ELEMENT *elem, INT side, VECTOR **vHandle);
CONNECTION *CreateConnection        (GRID *theGrid, VECTOR *from, VECTOR *to);
INT         CreateElementList        (GRID *theGrid, NODE *theNode, ELEMENT *theElement);
INT         DisposeVector            (GRID *theGrid, VECTOR *theVector);
INT         DisposeConnection        (GRID *theGrid, CONNECTION *theConnection);
/*@}*/

/** @name More create and dispose */
/*@{*/
INT                     MGCreateConnection                              (MULTIGRID *theMG);
INT             CreateConnectionsInNeighborhood (GRID *theGrid, ELEMENT *theElement);
INT             InsertedElementCreateConnection (GRID *theGrid, ELEMENT *theElement);
INT             DisposeConnectionFromVector     (GRID *theGrid, VECTOR *theVector);
INT             DisposeConnectionFromElement    (GRID *theGrid, ELEMENT *theElement);
INT             DisposeConnectionsInNeighborhood(GRID *theGrid, ELEMENT *theElement);
INT                     DisposeConnectionsFromMultiGrid (MULTIGRID *theMG);
#ifdef __THREEDIM__
INT             DisposeDoubledSideVector                (GRID *theGrid, ELEMENT *Elem0, INT Side0, ELEMENT *Elem1, INT Side1);
#endif
INT             DisposeElementList(GRID *theGrid, NODE *theNode);
INT             DisposeElementFromElementList (GRID *theGrid, NODE *theNode, ELEMENT *theElement);
/*@}*/

/** @name Query functions */
/*@{*/
INT             GetVectorsOfElement                     (const ELEMENT *theElement, INT *cnt, VECTOR **vList);
INT             GetVectorsOfSides                               (const ELEMENT *theElement, INT *cnt, VECTOR **vList);
INT             GetVectorsOfEdges                               (const ELEMENT *theElement, INT *cnt, VECTOR **vList);
INT             GetVectorsOfNodes                               (const ELEMENT *theElement, INT *cnt, VECTOR **vList);
INT                     GetVectorsOfOType                               (const ELEMENT *theElement, INT type, INT *cnt, VECTOR **vList);
INT                     DataTypeFilterVList                             (INT dt, VECTOR **vec, INT *cnt);
INT                     GetVectorsOfDataTypesInObjects  (const ELEMENT *theElement, INT dt, INT obj, INT *cnt, VECTOR *VecList[]);
INT                     PrepareGetBoundaryNeighbourVectors (GRID *theGrid, INT *MaxListLen);
INT                     ResetGetBoundaryNeighbourVectors (void);
INT                     GetBoundaryNeighbourVectors             (INT dt, INT obj, INT *cnt, VECTOR *VecList[]);
INT                     FinishBoundaryNeighbourVectors  (void);
INT             GetElementInfoFromSideVector    (const VECTOR *theVector, ELEMENT **Elements, INT *Sides);
#ifdef ModelP
INT         GetVectorSize                   (GRID *theGrid, INT VectorObjType, GEOM_OBJECT *object);
#endif
/*@}*/

/** @name Gridwise functions */
/*@{*/
INT             GridCreateConnection                    (GRID *theGrid);
INT             SetSurfaceClasses                               (MULTIGRID *theMG);
INT         CreateAlgebra                               (MULTIGRID *theMG);
/*@}*/

/** @name Check algebra */
/*@{*/
INT                     ElementCheckConnection                  (GRID *theGrid, ELEMENT *theElement);
INT             CheckAlgebra                                    (GRID *theGrid);
/*@}*/

/** @name Determination of vector classes */
/*@{*/
INT             ClearVectorClasses                              (GRID *theGrid);
INT             SeedVectorClasses                               (GRID *theGrid, ELEMENT *theElement);
INT             PropagateVectorClasses                  (GRID *theGrid);
INT             ClearNextVectorClasses                  (GRID *theGrid);
INT             SeedNextVectorClasses                   (GRID *theGrid, ELEMENT *theElement);
INT             PropagateNextVectorClasses              (GRID *theGrid);
INT             MaxNextVectorClass                              (GRID *theGrid, ELEMENT *theElement);
/*@}*/

/** @name Miscellaneous routines */
/*@{*/
INT             PrepareAlgebraModification              (MULTIGRID *theMG);
/*@}*/

/** \brief Initialization */
INT             InitAlgebra (void);

END_UGDIM_NAMESPACE

#endif

/** @} */
