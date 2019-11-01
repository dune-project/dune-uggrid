// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      disctools.h                                                   */
/*                                                                          */
/* Purpose:   tools for assembling (header file)                            */
/*                                                                          */
/* Author:    Christian Wieners                                             */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   Nov 27 95                                                     */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __DISCTOOLS__
#define __DISCTOOLS__

#include <dune/uggrid/gm/gm.h>
#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>
#include <dune/uggrid/numerics/np.h>

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

/* display data */
INT PrintVector                                 (GRID *g, const VECDATA_DESC *X, INT vclass, INT vnclass);
INT PrintVectorX                                (const GRID *g, const VECDATA_DESC *X, INT vclass, INT vnclass, PrintfProcPtr Printf);
INT PrintMatrix                                 (GRID *g, MATDATA_DESC *Mat, INT vclass, INT vnclass);

END_UGDIM_NAMESPACE

#endif
