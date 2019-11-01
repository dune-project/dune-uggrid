// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*	                                                                        */
/* File:      defaults.c                                                    */
/*                                                                          */
/* Purpose:   access to defaults file                                       */
/*                                                                          */
/* Author:      Peter Bastian                                               */
/*              Institut fuer Computeranwendungen III                       */
/*              Universitaet Stuttgart                                      */
/*              Pfaffenwaldring 27                                          */
/*              70569 Stuttgart                                             */
/*              email: ug@ica3.uni-stuttgart.de                             */
/*                                                                          */
/* History:   17.12.94 begin, ug version 3.0                                */
/*                                                                          */
/* Revision:  06.09.95                                                      */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*              system include files                                        */
/*              application include files                                   */
/*                                                                          */
/****************************************************************************/

#include <config.h>

#include "defaults.h"

USING_UG_NAMESPACE

/****************************************************************************/
/** \brief Provide access to defaults file

   This function provides access to defaults file. When 'ug' is started
   this function reads the defaults on file in order to set some
   parameters of 'ug' in advance.

   \return <ul>
   <li> 0 if OK </li>
   <li> 1 if error in opening or reading defaults file </li>
   </ul>

   \sa
   GetDefaultValue
 */
/****************************************************************************/

INT NS_PREFIX GetLocalizedDefaultValue (const char *, const char *, char *)
{
  return 1;
}

/****************************************************************************/
/** \brief Provide access to defaults file

   This function provides access to defaults file. When 'ug' is started
   this function reads the defaults on file in order to set some
   parameters of 'ug' in advance.

   \return <ul>
   <li> 0 if OK </li>
   <li> 1 if error in opening or reading defaults file </li>
   </ul>

   \sa
   GetLocalizedDefaultValue
 */
/****************************************************************************/

INT NS_PREFIX GetDefaultValue (const char *, const char *, char *)
{
  return 1;
}
