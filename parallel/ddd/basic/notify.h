// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      notify.h                                                      */
/*                                                                          */
/* Purpose:   header file for notification module                           */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/*                                                                          */
/* History:   95/04/04 kb  created                                          */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* auto include mechanism and other include files                           */
/*                                                                          */
/****************************************************************************/

#ifndef __DDD_NOTIFY_H__
#define __DDD_NOTIFY_H__

#include <dune/uggrid/parallel/ddd/dddtypes.hh>

#include "include/ddd.h"

namespace DDD {
namespace Basic {

struct NOTIFY_DESC
{
  DDD_PROC proc;
  size_t size;
};

} /* namespace Basic */
} /* namespace DDD */

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

#define EXCEPTION_NOTIFY  -1

/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
/*                                                                          */
/****************************************************************************/

using NOTIFY_DESC = DDD::Basic::NOTIFY_DESC;

/****************************************************************************/
/*                                                                          */
/* function declarations                                                    */
/*                                                                          */
/****************************************************************************/


void         NotifyInit(DDD::DDDContext& context);
void         NotifyExit(DDD::DDDContext& context);

NOTIFY_DESC *DDD_NotifyBegin(DDD::DDDContext& context, int);
void         DDD_NotifyEnd(DDD::DDDContext& context);
int          DDD_Notify(DDD::DDDContext& context);

END_UGDIM_NAMESPACE

#endif
