// SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
// SPDX-License-Identifier: LGPL-2.1-or-later
#ifndef DUNE_UGGRID_PARALLEL_DDD_DDDCONSTANTS_HH
#define DUNE_UGGRID_PARALLEL_DDD_DDDCONSTANTS_HH 1

#include <cstddef>

namespace DDD {

/** maximum number of TYPE_DESC */
static const int MAX_TYPEDESC = 32;

/** maximum number of interfaces */
static const std::size_t MAX_IF = 32;

/** maximum length of interface description string */
static const std::size_t IF_NAMELEN = 80;

/** size of segment of couplings (for memory allocation) */
static const std::size_t CPLSEGM_SIZE = 512;

} /* namespace DDD */

#endif
