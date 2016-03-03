set(SOURCES  jcmds.cc join.cc)

install(FILES join.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})

if(MPI_FOUND)
  ug_add_dim_libs(join OBJECT SOURCES ${SOURCES})
endif()
