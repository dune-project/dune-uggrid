// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file commands.c
 * \ingroup ui
 */

/** \addtogroup ui
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      commands.c                                                    */
/*                                                                          */
/* Purpose:   definition of all dimension independent commands of ug        */
/*                                                                          */
/* Author:    Henrik Rentz-Reichert                                         */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   29.01.92 begin, ug version 2.0                                */
/*            02.02.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:   for dimension dependent commands see commands2d/3d.c          */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/* system include files                                                     */
/* application include files                                                */
/*                                                                          */
/****************************************************************************/

/* own header */
#include "commands.h"

USING_UG_NAMESPACES

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

static MULTIGRID *currMG=NULL;                  /*!< The current multigrid			*/

REP_ERR_FILE

/****************************************************************************/
/** \brief Return a pointer to the current multigrid
 *
 * This function returns a pionter to the current multigrid.
 *
 * @return <ul>
 *    <li> pointer to multigrid </li>
 *    <li> NULL if there is no current multigrid. </li>
 * </ul>
 */
/****************************************************************************/

MULTIGRID * NS_DIM_PREFIX GetCurrentMultigrid (void)
{
  return (currMG);
}

/** @} */
