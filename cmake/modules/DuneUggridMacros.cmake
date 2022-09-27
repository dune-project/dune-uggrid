# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Compatibility against previous UG versions.
set(UG_FOUND True)
set(UG_VERSION "${DUNE_UGGRID_VERSION}")

set(UG_DEFINITIONS)
if(UG_PARALLEL)
  list(APPEND UG_DEFINITIONS "ModelP")
endif()
