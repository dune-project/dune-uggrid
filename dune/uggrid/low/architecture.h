// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// SPDX-FileCopyrightInfo: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later

/*

   provides, based on config.h, a consistent interface for system
   dependent stuff

 */

#ifndef UG_ARCHITECTURE_H
#define UG_ARCHITECTURE_H

/* SMALL..: least number s.t. 1 + SMALL../SMALL_FAC != 1 */
#define SMALL_FAC 10

#include <float.h>
#define MAX_F            FLT_MAX
#define SMALL_F         (FLT_EPSILON*SMALL_FAC)
#define MAX_D            DBL_MAX
#define SMALL_D         (DBL_EPSILON*SMALL_FAC)
#define MAX_C            FLT_MAX
#define SMALL_C         (FLT_EPSILON*SMALL_FAC)

/* data alignment of 8 should suffice on all architecture */
/* !!! set after testing? */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) ! */

#endif
