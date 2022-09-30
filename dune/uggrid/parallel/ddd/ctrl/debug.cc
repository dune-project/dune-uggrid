// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      debug.c                                                       */
/*                                                                          */
/* Purpose:   produces lists for debugging DDD data structures              */
/*                                                                          */
/* Author:    Klaus Birken                                                  */
/*            Rechenzentrum Uni Stuttgart                                   */
/*            Universitaet Stuttgart                                        */
/*            Allmandring 30                                                */
/*            70550 Stuttgart                                               */
/*            internet: birken@rus.uni-stuttgart.de                         */
/*                                                                          */
/* History:   94/07/04 kb  begin                                            */
/*                                                                          */
/* Remarks:                                                                 */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* include files                                                            */
/*            system include files                                          */
/*            application include files                                     */
/*                                                                          */
/****************************************************************************/

/* standard C library */
#include <config.h>
#include <cstdlib>
#include <cstdio>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <tuple>

#include <dune/uggrid/parallel/ddd/dddi.h>

/* PPIF namespace: */
using namespace PPIF;

START_UGDIM_NAMESPACE

/****************************************************************************/
/*                                                                          */
/* defines in the following order                                           */
/*                                                                          */
/*        compile time constants defining static data size (i.e. arrays)    */
/*        other constants                                                   */
/*        macros                                                            */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* data structures                                                          */
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




/****************************************************************************/
/*                                                                          */
/* routines                                                                 */
/*                                                                          */
/****************************************************************************/



/****************************************************************************/
/*                                                                          */
/* Function:  ListLocalObjects                                              */
/*                                                                          */
/* Purpose:   display list of all local objects                             */
/*                                                                          */
/* Input:     -                                                             */
/*                                                                          */
/* Output:    -                                                             */
/*                                                                          */
/****************************************************************************/

static bool sort_LocalObjs(const DDD_HDR& a, const DDD_HDR& b)
{
  return std::tie(OBJ_TYPE(a), OBJ_GID(a)) < std::tie(OBJ_TYPE(b), OBJ_GID(b));
}


void DDD_ListLocalObjects(const DDD::DDDContext& context)
{
  using std::setw;
  std::ostream& out = std::cout;

  std::vector<DDD_HDR> locObjs = LocalObjectsList(context);
  if (locObjs.empty())
    return;

  std::sort(locObjs.begin(), locObjs.end(), sort_LocalObjs);

  for(int i=0; i < context.nObjs(); i++)
  {
    const auto& o = locObjs[i];

    out << "#" << setw(4) << "  adr=" << o << " gid=" << OBJ_GID(o)
        << " type=" << OBJ_TYPE(o) << " prio=" << OBJ_PRIO(o)
        << " attr=" << OBJ_ATTR(o) << "\n";
  }
}

END_UGDIM_NAMESPACE
