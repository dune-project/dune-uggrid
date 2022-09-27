# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

target_sources_dims(duneuggrid PRIVATE
  std_domain.cc
  std_parallel.cc)

install(FILES
  domain.h
  std_domain.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/domain)
