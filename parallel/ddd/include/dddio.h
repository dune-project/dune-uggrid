// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      dddio.h                                                       */
/*                                                                          */
/* Purpose:   I/O routines used by DDD                                      */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/*                                                                          */
/* History:   30.01.92 begin, ug version 2.0 (Peter Bastian)                */
/*            93/11/26 kb  PrintErrorMessage copied from ug version 2.0     */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __DDDIO__
#define __DDDIO__

#include <dune/uggrid/parallel/ddd/dddtypes.hh>

namespace DDD {

/****************************************************************************/
/*                                                                          */
/* user-defined lineout-function prototype                                  */
/*                                                                          */
/****************************************************************************/

extern void (*DDD_UserLineOutFunction)(const char *s);


/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/

void DDD_PrintLine (const char *s);
void DDD_PrintDebug (const char *s);
void DDD_PrintError (char error_class, int error_no, const char *text);
void DDD_Flush (void);
void DDD_SyncAll(const DDD::DDDContext& context);

} /* namespace DDD */

#endif
