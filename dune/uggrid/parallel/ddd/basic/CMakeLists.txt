# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(test)

target_sources(duneuggrid PRIVATE
  io.cc
  lowcomm.cc
  notify.cc
  reduct.cc
  topo.cc)

install(FILES notify.h lowcomm.h oopp.h ooppcc.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/basic)
