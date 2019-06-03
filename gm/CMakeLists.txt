set(SOURCES  algebra.cc enrol.cc evm.cc mgio.cc
  ugio.cc ugm.cc cw.cc initgm.cc elements.cc
  shapes.cc evalproc.cc rm.cc refine.cc
  dlmgr.cc gmcheck.cc er.cc mgheapmgr.cc rm-write2file.cc )
# put this "template" into distribution-tarball as well
set(EXTRA_DIST  dlmgr.t)
set(gminclude_HEADERS  cw.h elements.h gm.h pargm.h evm.h
  shapes.h ugm.h dlmgr.h algebra.h rm.h refine.h rm-write2file.h)

ug_add_dim_libs(ug_gm OBJECT SOURCES ${SOURCES})# OBJECT_DIM_LIBS gg)

dune_add_test(
  NAME rm3-tetrahedron-rules-test
  SOURCES rm-tetrahedron-rules-test.cc
  COMPILE_DEFINITIONS -DUG_DIM_3
  LINK_LIBRARIES ugL ugS3 ${DUNE_LIBS}
  )

# rm3-show
add_executable(rm3-show rm-show.cc)
target_compile_definitions(rm3-show PRIVATE -DUG_DIM_3)
target_link_libraries(rm3-show PRIVATE ugL ugS3 ${DUNE_LIBS})

# rm3-writeRefRules2file
add_executable(rm3-writeRefRules2file rm-writeRefRules2file.cc rm-write2file.h rm-write2file.cc)
target_compile_definitions(rm3-writeRefRules2file PRIVATE -DUG_DIM_3)
target_link_libraries(rm3-writeRefRules2file PRIVATE ugL ugS3 ${DUNE_LIBS})

install(FILES ${gminclude_HEADERS} DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
