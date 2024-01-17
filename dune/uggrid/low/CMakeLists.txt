# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(test)

target_sources(duneuggrid PRIVATE
  bio.cc
  debug.cc
  fifo.cc
  fileopen.cc
  heaps.cc
  initlow.cc
  misc.cc
  ugenv.cc)

install(FILES
  debug.h
  dimension.h
  fileopen.h
  heaps.h
  misc.h
  namespace.h
  ugenv.h
  ugtypes.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/low)
