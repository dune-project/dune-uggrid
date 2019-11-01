// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*! \file er.h
 * \ingroup gm
 */

/****************************************************************************/
/*                                                                                                                                                      */
/* File:          er.h                                                                                                                  */
/*                                                                                                                                                      */
/* Purpose:   extract rules realized in a multigrid                                             */
/*                                                                                                                                                      */
/* Author:        Henrik Rentz-Reichert                                                                                 */
/*                        Institut fuer Computeranwendungen III                                                 */
/*                        Universitaet Stuttgart                                                                                */
/*                        Pfaffenwaldring 27                                                                                    */
/*                        70550 Stuttgart                                                                                               */
/*                        email: ug@ica3.uni-stuttgart.de                                                               */
/*                                                                                                                                                      */
/* History:   06.12.97 begin, ug version 3.7                                                            */
/*                                                                                                                                                      */
/* Remarks:                                                                                                                             */
/*                                                                                                                                                      */
/****************************************************************************/


/****************************************************************************/
/*                                                                                                                                                      */
/* auto include mechanism and other include files                                                       */
/*                                                                                                                                                      */
/****************************************************************************/

#ifndef __ER__
#define __ER__

#include "mgio.h"

#include <dune/uggrid/low/namespace.h>

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                                                                                                      */
/* defines in the following order                                                                                       */
/*                                                                                                                                                      */
/*                compile time constants defining static data size (i.e. arrays)        */
/*                other constants                                                                                                       */
/*                macros                                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* data structures exported by the corresponding source file                            */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* definition of exported global variables                                                                      */
/*                                                                                                                                                      */
/****************************************************************************/



/****************************************************************************/
/*                                                                                                                                                      */
/* function declarations                                                                                                        */
/*                                                                                                                                                      */
/****************************************************************************/

INT GetOrderedSons                                              (ELEMENT *theElement, MGIO_RR_RULE *theRule, NODE **NodeContext, ELEMENT **SonList, INT *nmax);
INT NEW_Write_RefRules                                  (MULTIGRID *mg, INT RefRuleOffset[], INT MarkKey, MGIO_RR_RULE **mrule_handle);
INT ResetRefineTagsBeyondRuleManager    (MULTIGRID *mg);

END_UGDIM_NAMESPACE

#endif
