# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(include)

install(FILES
  dddconstants.hh
  dddcontext.hh
  dddtypes.hh
  dddtypes_impl.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd)

if(UG_ENABLE_PARALLEL)
  add_subdirectory(analyser)
  add_subdirectory(basic)
  add_subdirectory(ctrl)
  add_subdirectory(ident)
  add_subdirectory(if)
  add_subdirectory(join)
  add_subdirectory(mgr)
  add_subdirectory(prio)
  add_subdirectory(xfer)

  target_sources_dims(duneuggrid PRIVATE ddd.cc)
endif()

target_sources(duneuggrid PRIVATE dddcontext.cc)
