// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      scan.c                                                        */
/*                                                                          */
/* Purpose:   tools for reading script arguments                            */
/*                                                                          */
/*                                                                          */
/* Author:    Christian Wieners                                             */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/* email:     ug@ica3.uni-stuttgart.de                                      */
/*                                                                          */
/* History:   November 23, 1996                                             */
/*            low part of former np/udm/scan.c, 15.5.97                     */
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

#include <config.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "general.h"
#include "debug.h"
#include "ugenv.h"
#include "misc.h"
#include "scan.h"

USING_UG_NAMESPACE

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
/*                                                                          */
/* data structures used in this source file (exported data structures are   */
/* in the corresponding include file!)                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of exported global variables                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* definition of variables global to this source file only (static!)        */
/*                                                                          */
/****************************************************************************/

  REP_ERR_FILE


/****************************************************************************/
/*                                                                          */
/* forward declarations of functions used before they are defined           */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/** \brief Read command line doubles

   \param name - name of the argument
   \param a - DOUBLE value
   \param argc - argument counter
   \param argv - argument vector

   This function reads command strings and returns a DOUBLE value.

   \return <ul>
   <li>  0 if the argument was found and a DOUBLE value could be read </li>
   <li>  1 else. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX ReadArgvDOUBLE (const char *name, DOUBLE *a, INT argc, char **argv)
{
  INT i;
  char option[OPTIONLEN];
  double value;

  for (i=0; i<argc; i++)
    if (argv[i][0]==name[0])
    {
      if (sscanf(argv[i],"%s %lf",option,&value)!=2)
        continue;
      if (strcmp(option,name) == 0)
      {
        a[0] = value;
        return(0);
      }
    }

  return (1);
}

/****************************************************************************/
/** \brief Read command line options

   \param name - name of the argument
   \param argc - argument counter
   \param argv - argument vector

   This function reads command strings and returns an integer value.

   \return <ul>
   <li>  0 if the option is not set </li>
   <li>  n if an integer n is given with the option </li>
   <li>  1 else. </li>
   </ul>
 */
/****************************************************************************/

INT NS_PREFIX ReadArgvOption (const char *name, INT argc, char **argv)
{
  INT i;
  char option[OPTIONLEN];
  int value;

  for (i=0; i<argc; i++)
    if (argv[i][0]==name[0])
    {
      if (sscanf(argv[i],"%s %d",option,&value) == 2)
        if (strcmp(option,name) == 0)
          return(value);
      if (strcmp(argv[i],name) == 0)
        return(1);
    }

  return (0);
}
