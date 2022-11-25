# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

if(UG_ENABLE_PARALLEL)
  target_sources(duneuggrid PRIVATE ppif.cc)
endif()

target_sources(duneuggrid PRIVATE ppifcontext.cc)

install(FILES
  ppif.h
  ppifcontext.hh
  ppiftypes.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ppif)
