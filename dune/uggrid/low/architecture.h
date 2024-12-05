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

#include <limits>
constexpr float MAX_F =    std::numeric_limits<float>::max();
constexpr float SMALL_F =  std::numeric_limits<float>::epsilon() * SMALL_FAC;
constexpr float MAX_C =    std::numeric_limits<float>::max();
constexpr float SMALL_C =  std::numeric_limits<float>::epsilon() * SMALL_FAC;
constexpr double MAX_D =   std::numeric_limits<double>::max();
constexpr double SMALL_D = std::numeric_limits<double>::epsilon() * SMALL_FAC;

/* data alignment of 8 should suffice on all architecture */
/* !!! set after testing? */
#define ALIGNMENT 8                     /* power of 2 and >= sizeof(int) ! */

#endif
