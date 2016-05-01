set(UG_3DLIBS ugS2 devS)
set(UG_2DLIBS ugS3 devS)
# for plugins later:
#
# !!! dynamic options should be global or so... or in the lib?
# AM_LDFLAGS = -dlopen self
# xugv needs X11 and XAW, the latter includes the first
if( XAW)
 set(XUGV  xugv)
endif( XAW )

if(UG_ENABLE_2D)
  set(TOOLS_2D  ugmgs2 ugpfm2)
endif()

if(UG_ENABLE_3D)
  set(TOOLS_3D  ugmgs3 ugpfm3)
endif()

set(TOOLS  ugnetgen ugstl ${XUGV} ugmesh m2ps)
set(PROGRAMS  ${TOOLS} ${TOOLS_2D} ${TOOLS_3D})

# !!! std-domain / device SIF ok?
set(UG_2DLIBS  ugS2 devS)
set(UG_3DLIBS  ugS3 devS)

add_executable(ugnetgen ugnetgen.cc)
target_link_libraries(ugnetgen ${UG_2DLIBS})
target_compile_definitions(ugnetgen PRIVATE "-DUG_DIM_2")

add_executable(ugstl ugstl.cc)
target_link_libraries(ugstl ${UG_2DLIBS})
target_compile_definitions(ugstl PRIVATE "-DUG_DIM_2")

if(XAW)
  add_executable(xugv xugv.cc)
  add_dune_ug_flags(xugv )
  target_link_libraries(xugv ${UG_2DLIBS})
  target_compile_definitions(xugv PRIVATE "-DUG_DIM_2")
  # The following lines are comment out as the translation is not clear. TODO review
  #
  # set(xugv_ LDFLAGS ${UG_XLIBS})
  # The following lines are comment out as the translation is not clear. TODO review
  #
  # set(xugv_ LDADD ${UG_2DLIBS})
endif()

add_executable(ugmesh ugmesh.cc)
target_link_libraries(ugmesh ${UG_2DLIBS})
target_compile_definitions(ugmesh PRIVATE "-DUG_DIM_2")

add_executable(m2ps m2ps.cc)
target_link_libraries(m2ps ${UG_2DLIBS})
target_compile_definitions(m2ps PRIVATE "-DUG_DIM_2")


if(UG_ENABLE_2D)
  add_executable(ugpfm2 ugpfm.cc)
  target_link_libraries(ugpfm2 ${UG_2DLIBS})
  target_compile_definitions(ugpfm2 PRIVATE "-DUG_DIM_2")

  add_executable(ugmgs2 ugmgs.cc)
  target_link_libraries(ugmgs2 ${UG_2DLIBS})
  target_compile_definitions(ugmgs2 PRIVATE "-DUG_DIM_2")
endif()

if(UG_ENABLE_3D)
  add_executable(ugpfm3 ugpfm.cc)
  target_link_libraries(ugpfm3 ${UG_3DLIBS})
  target_compile_definitions(ugpfm3 PRIVATE "-DUG_DIM_3")

  add_executable(ugmgs3 ugmgs.cc)
  target_link_libraries(ugmgs3 ${UG_3DLIBS})
  target_compile_definitions(ugmgs3 PRIVATE "-DUG_DIM_3")
endif()

add_dune_mpi_flags(${PROGRAMS})

install(TARGETS ${PROGRAMS} DESTINATION ${CMAKE_INSTALL_BINDIR})
