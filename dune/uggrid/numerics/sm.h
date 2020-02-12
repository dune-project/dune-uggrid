// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*                                                                          */
/* File:      sm.h                                                          */
/*                                                                          */
/* Purpose:   interfaces to sparse matrix handling routines                 */
/*                                                                          */
/* Author:    Nicolas Neuss                                                 */
/*            email: Nicolas.Neuss@IWR.Uni-Heidelberg.De                    */
/*                                                                          */
/* History:   01.98 begin sparse matrix routines                            */
/*                                                                          */
/* Note:      The files sm.[ch], blasm.[ch] may be obtained also in a       */
/*            standalone form under the GNU General Public License.         */
/*            The use inside of UG under the actual UG license              */
/*            is allowed.                                                   */
/*                                  HD, 13.7.99,  Nicolas Neuss.            */
/*                                                                          */
/****************************************************************************/

#ifndef __SM__
#define __SM__

#include <dune/uggrid/low/namespace.h>
#include <dune/uggrid/low/ugtypes.h>

#include <cstddef>

START_UGDIM_NAMESPACE

/****************************************************************************/
/*																			*/
/* Routines for sparse matrix handling                                                                  */
/*																			*/
/****************************************************************************/

INT String2SMArray       (SHORT n, char *str, SHORT *comps);

END_UGDIM_NAMESPACE

#endif /* __SM__ */
