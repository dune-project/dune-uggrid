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

/** @name basic create and dispose functions */
/*@{*/
INT         CreateSideVector                (GRID *theGrid, INT side, GEOM_OBJECT *object, VECTOR **vHandle);
INT         CreateElementList        (GRID *theGrid, NODE *theNode, ELEMENT *theElement);
INT         DisposeVector            (GRID *theGrid, VECTOR *theVector);
/*@}*/

/** @name More create and dispose */
/*@{*/
#ifdef UG_DIM_3
INT             DisposeDoubledSideVector                (GRID *theGrid, ELEMENT *Elem0, INT Side0, ELEMENT *Elem1, INT Side1);
#endif
INT             DisposeElementList(GRID *theGrid, NODE *theNode);
INT             DisposeElementFromElementList (GRID *theGrid, NODE *theNode, ELEMENT *theElement);
/*@}*/

/** @name Query functions */
/*@{*/
INT             GetVectorsOfSides                               (const ELEMENT *theElement, INT *cnt, VECTOR **vList);
INT                     GetVectorsOfOType                               (const ELEMENT *theElement, INT type, INT *cnt, VECTOR **vList);
INT                     DataTypeFilterVList                             (INT dt, VECTOR **vec, INT *cnt);
INT                     GetVectorsOfDataTypesInObjects  (const ELEMENT *theElement, INT dt, INT obj, INT *cnt, VECTOR *VecList[]);
INT             GetElementInfoFromSideVector    (const VECTOR *theVector, ELEMENT **Elements, INT *Sides);
#ifdef ModelP
INT         GetVectorSize                   (GRID *theGrid, INT VectorObjType, GEOM_OBJECT *object);
#endif
/*@}*/

/** @name Gridwise functions */
/*@{*/
INT             SetSurfaceClasses                               (MULTIGRID *theMG);
INT         CreateAlgebra                               (MULTIGRID *theMG);
/*@}*/

/** @name Check algebra */
/*@{*/
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
/*@}*/

/** @name Miscellaneous routines */
/*@{*/
INT             PrepareAlgebraModification              (MULTIGRID *theMG);
/*@}*/

END_UGDIM_NAMESPACE

#endif

/** @} */
