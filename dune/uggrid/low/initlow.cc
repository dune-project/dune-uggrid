// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file initlow.c
 * \ingroup low
 */

/** \addtogroup low
 *
 * @{
 */

/****************************************************************************/
/*                                                                          */
/* File:      initlow.c                                                     */
/*                                                                          */
/* Purpose:   call the init routines of the low module                      */
/*                                                                          */
/* Author:    Henrik Rentz-Reichert                                         */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   27.02.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/* system include files                                                     */
/* application include files                                                */
/*                                                                          */
/****************************************************************************/

/* ANSI-C includes */
#include <config.h>
#include <cstdio>

/* low module */
#include "ugtypes.h"
#include "misc.h"
#include "heaps.h"
#include "ugenv.h"
#include "fileopen.h"

/* own header */
#include "initlow.h"

USING_UG_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/




#ifdef ModelP
#define DEFAULTENVSIZE  512000  /* size of environment if no default value	*/
#else
#define DEFAULTENVSIZE  128000  /* size of environment if no default value	*/
#endif

/****************************************************************************/
/** \brief Call the inits for the low module
 *
 * This function calls the inits for the low module.
 *
 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error occurred </li>
 * </ul>
 */
/****************************************************************************/
INT NS_PREFIX InitLow ()
{
  INT err;

  /* init ugenv.c */
  if ((err=InitUgEnv())!=0)
  {
    SetHiWrd(err,__LINE__);
    return (err);
  }

  /* init fileopen */
  if ((err=InitFileOpen())!=0)
  {
    SetHiWrd(err,__LINE__);
    return (err);
  }

  return (0);
}



/****************************************************************************/
/** \brief Call the exits for the low module
 *
 * This function calls the exit methods for the low module.
 *
 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error occurred </li>
 * </ul>
 */
/****************************************************************************/
INT NS_PREFIX ExitLow ()
{
  INT err;

  /* exit env */
  if ((err=ExitUgEnv())!=0)
  {
    SetHiWrd(err,__LINE__);
    return (err);
  }

  return (0);
}

/** @} */
