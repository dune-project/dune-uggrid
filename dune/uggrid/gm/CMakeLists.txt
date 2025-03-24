# SPDX-FileCopyrightText: Copyright © DUNE Project contributors, see file LICENSE.md in module root
# SPDX-License-Identifier: LGPL-2.1-or-later

target_sources_dims(duneuggrid PRIVATE
  algebra.cc
  cw.cc
  dlmgr.cc
  elements.cc
  er.cc
  evm.cc
  gmcheck.cc
  initgm.cc
  mgio.cc
  refine.cc
  rm-write2file.cc
  rm.cc
  shapes.cc
  ugm.cc)

dune_add_test(
  NAME rm3-tetrahedron-rules-test
  SOURCES rm-tetrahedron-rules-test.cc
  COMPILE_DEFINITIONS -DUG_DIM_3
  LINK_LIBRARIES duneuggrid ${DUNE_LIBS}
  CMAKE_GUARD DUNE_UGGRID_TET_RULESET
  )

# rm3-show
add_executable(rm3-show rm-show.cc)
target_compile_definitions(rm3-show PRIVATE -DUG_DIM_3)
target_link_libraries(rm3-show PRIVATE duneuggrid ${DUNE_LIBS})

# rm3-writeRefRules2file
add_executable(rm3-writeRefRules2file rm-writeRefRules2file.cc rm-write2file.h rm-write2file.cc)
target_compile_definitions(rm3-writeRefRules2file PRIVATE -DUG_DIM_3)
target_link_libraries(rm3-writeRefRules2file PRIVATE duneuggrid ${DUNE_LIBS})

install(FILES
  algebra.h
  cw.h
  dlmgr.h
  elements.h
  evm.h
  gm.h
  pargm.h
  refine.h
  rm-write2file.h
  rm.h
  shapes.h
  ugm.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/gm)
