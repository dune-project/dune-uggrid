// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/*! \file ugio.h
 * \ingroup gm
 */


/****************************************************************************/
/*                                                                                                                                                      */
/* File:          ugio.h                                                                                                                */
/*                                                                                                                                                      */
/* Purpose:   ug input/output header file                                       */
/*                                                                                                                                                      */
/* Author:        Peter Bastian                                                                                                 */
/*                        Interdisziplinaeres Zentrum fuer Wissenschaftliches Rechnen   */
/*                        Universitaet Heidelberg                                                                               */
/*                        Im Neuenheimer Feld 368                                                                               */
/*                        6900 Heidelberg                                                                                               */
/*                        internet: ug@ica3.uni-stuttgart.de                                            */
/*                                                                                                                                                      */
/* History:   15.04.92 begin, ug version 2.0                                                            */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/


/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __UGIO__
#define __UGIO__

#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

#include "gm.h"

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

INT InitUgio ();

END_UGDIM_NAMESPACE

#endif
