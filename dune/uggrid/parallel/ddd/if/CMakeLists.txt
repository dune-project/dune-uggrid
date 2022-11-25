# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

target_sources_dims(duneuggrid PRIVATE
  ifcheck.cc
  ifcmds.cc
  ifcreate.cc
  ifobjsc.cc
  ifuse.cc)

install(FILES
  if.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/if)
