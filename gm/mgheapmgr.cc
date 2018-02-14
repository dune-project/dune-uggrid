// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      mgheapmgr.c                                                   */
/*                                                                          */
/* Purpose:   mg heap manager                                               */
/*                                                                          */
/* Author:	  Stefan Lang                                                                                   */
/*			  Institut fuer Computeranwendungen III                                                 */
/*			  Universitaet Stuttgart										*/
/*			  Pfaffenwaldring 27											*/
/*			  70550 Stuttgart												*/
/*			  email: ug@ica3.uni-stuttgart.de								*/
/*																			*/
/* History:   980826, start                                                                             */
/*																			*/
/* Remarks: controls de/allocation of bottom heap memory                    */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* include files															*/
/*			  system include files											*/
/*			  application include files                                                                     */
/*																			*/
/****************************************************************************/

#include <config.h>
#include "general.h"
#include "ugtypes.h"
#include "heaps.h"
#include "gm.h"
#include "algebra.h"
#include "mgheapmgr.h"

USING_UG_NAMESPACES

/****************************************************************************/
/*																			*/
/* defines in the following order											*/
/*																			*/
/*		  compile time constants defining static data size (i.e. arrays)	*/
/*		  other constants													*/
/*		  macros															*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* data structures used in this source file (exported data structures are	*/
/*		  in the corresponding include file!)								*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/
/* definition of variables global to this source file only (static!)		*/
/*																			*/
/****************************************************************************/

REP_ERR_FILE

/****************************************************************************/
/*																			*/
/* forward declarations of functions used before they are defined			*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/** \brief Dispose memory allocated tmp from bottom

   PARAMETERS:
   .  theMG - multigrid to handle

   DESCRIPTION:
   Disposes all memory which allocated temporarily from bottom.

   RETURN VALUE:

   SEE ALSO:
   D*/
/****************************************************************************/

INT NS_DIM_PREFIX DisposeBottomHeapTmpMemory (MULTIGRID *theMG)
{

  if (DisposeAMGLevels(theMG)) REP_ERR_RETURN(1);
  if (DisposeConnectionsFromMultiGrid(theMG)) REP_ERR_RETURN(1);

  theMG->bottomtmpmem = 0;
  if (Release(MGHEAP(theMG),FROM_BOTTOM,MGHEAP(theMG)->freelist_end_mark))
    REP_ERR_RETURN(1);
  MGHEAP(theMG)->freelist_end_mark = 0;

  return(0);
}
