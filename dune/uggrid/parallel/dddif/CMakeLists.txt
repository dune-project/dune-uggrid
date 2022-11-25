# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

target_sources_dims(duneuggrid PRIVATE
  compat.cc
  debugger.cc
  gridcons.cc
  handler.cc
  identify.cc
  identify.h
  initddd.cc
  lb.cc
  lbrcb.cc
  memmgr.cc
  overlap.cc
  partition.cc
  pgmcheck.cc
  priority.cc
  support.cc
  trans.cc)

install(FILES parallel.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/dddif/)
