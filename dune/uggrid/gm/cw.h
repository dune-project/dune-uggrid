// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************/
/*                                                                          */
/* File:      cw.h                                                          */
/*                                                                          */
/* Purpose:   control word definitions header file                          */
/*                                                                          */
/* Author:    Peter Bastian                                                 */
/*            Institut fuer Computeranwendungen III                         */
/*            Universitaet Stuttgart                                        */
/*            Pfaffenwaldring 27                                            */
/*            70569 Stuttgart                                               */
/*            email: ug@ica3.uni-stuttgart.de                               */
/*                                                                          */
/* History:   11.01.95 begin, ug version 3.0                                */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __CW__
#define __CW__

#include <dune/uggrid/low/namespace.h>

START_UGDIM_NAMESPACE

/** @name Macros for the control word management */
/*@{*/
/** \brief max number of entries                                */
#define MAX_CONTROL_ENTRIES 100
/*@}*/

/** \brief Manage part of a control word */
typedef struct {

  /** \brief this struct is used                          */
  INT used;

  /** \brief name string */
  const char *name;

  /** \brief pointer to corresponding control word */
  INT control_word;

  /** \brief shift in control word */
  INT offset_in_word;

  /** \brief number of bits used */
  INT length;

  /** \brief bitwise object ID  */
  INT objt_used;

  /** \brief copy from control word (faster)      */
  UINT offset_in_object;

  /** \brief 1 where bits are used                        */
  UINT mask;

  /** \brief 0 where bits are used                        */
  UINT xor_mask;

} CONTROL_ENTRY;

/** \brief Predefined control words */
extern CONTROL_ENTRY
  control_entries[MAX_CONTROL_ENTRIES];

/****************************************************************************/
/*                                                                          */
/* function definitions                                                     */
/*                                                                          */
/****************************************************************************/

INT InitCW      (void);

END_UGDIM_NAMESPACE

#endif
