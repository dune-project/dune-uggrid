// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file mgheapmgr.h
 * \ingroup gm
 */

/****************************************************************************/
/*                                                                                                                                                      */
/* File:          mgheapmgr.h                                                                                                   */
/*                                                                                                                                                      */
/* Purpose:   multigrid heap manager                                                    */
/*                                                                                                                                                      */
/* Author:        Stefan Lang                                                   */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70550 Stuttgart                                                                                               */
/*                        email: ug@ica3.uni-stuttgart.de                                                               */
/*                                                                                                                                                      */
/* History:   980826  start                                                 */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/


/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __MGHEAPMGR__
#define __MGHEAPMGR__


#include "namespace.h"

START_UGDIM_NAMESPACE

#ifdef DYNAMIC_MEMORY_ALLOCMODEL

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

INT DisposeBottomHeapTmpMemory (MULTIGRID *theMG);

#endif

END_UGDIM_NAMESPACE

#endif
