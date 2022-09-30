# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(ddd)
add_subdirectory(ppif)

if(UG_ENABLE_PARALLEL)
  add_subdirectory(dddif)
endif()
