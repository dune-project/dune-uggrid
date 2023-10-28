// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file
 * \ingroup ug
 */

/** \addtogroup ug
 *
 * @{
 */


/****************************************************************************/
/*                                                                          */
/* File:      initug.c                                                      */
/*                                                                          */
/* Purpose:   call the init routines of the ug modules                      */
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
/*    system include files                                                  */
/*    application include files                                             */
/*                                                                          */
/****************************************************************************/

/* ANSI-C includes */
#include <config.h>
#include <cstdio>
#include <cstring>

/* low module */
#include <dune/uggrid/low/initlow.h>
#include <dune/uggrid/low/misc.h>

/* parallelization module */
#ifdef ModelP
#include <dune/uggrid/parallel/dddif/parallel.h>
#include <dune/uggrid/parallel/ppif/ppif.h>
using namespace PPIF;
#endif

/* devices module */
#include <dune/uggrid/ugdevices.h>

/* domain module */
#include <dune/uggrid/domain/domain.h>

/* grid manager module */
#include <dune/uggrid/gm/initgm.h>

/* own header */
#include "initug.h"

/** \todo delete this */
#include <dune/uggrid/low/debug.h>


USING_UG_NAMESPACES

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/

#define UGDEBUGRFILE            "debugfile"

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/** \brief Call the init functions for all the ug modules
 *
 * @param argcp - pointer to argument counter
 * @param argvp - pointer to argument vector
 *
 *   This function initializes.
 *
 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error occurred. </li>
 * </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX InitUg (int *argcp, char ***argvp)
{
  INT err;

#ifdef ModelP
  /* init ppif module */
  if ((err = InitPPIF (argcp, argvp)) != PPIF_SUCCESS)
  {
    printf ("ERROR in InitParallel while InitPPIF.\n");
    printf ("aborting ug\n");

    return (1);
  }
#endif

  /* init the low module */
  if ((err = InitLow ()) != 0)
  {
    printf
      ("ERROR in InitUg while InitLow (line %d): called routine line %d\n",
      (int) HiWrd (err), (int) LoWrd (err));
    printf ("aborting ug\n");

    return (1);
  }

  /* init the devices module */
  if ((err = InitDevices()) != 0)
  {
    printf
      ("ERROR in InitUg while InitDevices (line %d): called routine line %d\n",
      (int) HiWrd (err), (int) LoWrd (err));
    printf ("aborting ug\n");

    return (1);
  }

#ifdef Debug
  {
    int i;
    for (i = 1; i < *argcp; i++)
      if (strncmp ((*argvp)[i], "-dbgfile", 8) == 0)
        break;
    if (i < *argcp)
    {
      const char* debugfilename = "dune-uggrid.dbg";
      if (SetPrintDebugToFile (debugfilename) != 0)
      {
        printf ("ERROR while opening debug file '%s'\n", debugfilename);
        printf ("aborting ug\n");
        return (1);
      }
      UserWriteF ("debug info is captured to file '%s'\n", debugfilename);
    }
    else
    {
      SetPrintDebugProc (printf);
      UserWriteF ("debug info is printed to stdout\n");
    }
  }
#endif

  /* init the domain module */
  if ((err = InitDom ()) != 0)
  {
    printf
      ("ERROR in InitDom while InitDom (line %d): called routine line %d\n",
      (int) HiWrd (err), (int) LoWrd (err));
    printf ("aborting ug\n");

    return (1);
  }

  /* init the gm module */
  if ((err = InitGm ()) != 0)
  {
    printf
      ("ERROR in InitUg while InitGm (line %d): called routine line %d\n",
      (int) HiWrd (err), (int) LoWrd (err));
    printf ("aborting ug\n");

    return (1);
  }

  return (0);
}


/****************************************************************************/
/** \brief Call of the exitfunctions for all the ug modules
 *
 * This function exits ug. It is called at the end of the CommandLoop.
 * It calls all available exit functions in reverse order of the corresponding
 * calls in InitUg().
 *
 * @return <ul>
 *   <li> 0 if ok </li>
 *   <li> 1 if error occurred. </li>
 * </ul>
 */
/****************************************************************************/

INT NS_DIM_PREFIX
ExitUg (void)
{
  INT err;

  /* exit gm module */
  PRINTDEBUG (init, 1, ("     ExitGm()...\n"))
  if ((err = ExitGm ()) != 0)
  {
    printf
      ("ERROR in ExitUg while ExitGm (line %d): called routine line %d\n",
      (int) HiWrd (err), (int) LoWrd (err));
    printf ("aborting ug\n");

    return (1);
  }

  /* exit devices module */
  PRINTDEBUG (init, 1, ("     ExitDevices()...\n"))
  if ((err = ExitDevices ()) != 0)
  {
    printf
      ("ERROR in ExitUg while ExitDevices (line %d): called routine line %d\n",
      (int) HiWrd (err), (int) LoWrd (err));
    printf ("aborting ug\n");

    return (1);
  }

  /* exit low module */
  PRINTDEBUG (init, 1, ("     ExitLow()...\n"))
  if ((err = ExitLow ()) != 0)
  {
    printf
      ("ERROR in ExitUg while ExitLow (line %d): called routine line %d\n",
      (int) HiWrd (err), (int) LoWrd (err));
    printf ("aborting ug\n");

    return (1);
  }

  return (0);
}

/** @} */
