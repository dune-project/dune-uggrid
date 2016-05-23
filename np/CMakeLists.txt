set(SOURCES  initnp.cc)# initnp.h initnumerics.h np.h num.h)

# famglib ist auch im alten Makefile nicht drin
add_subdirectory(algebra)
add_subdirectory(amglib)
add_subdirectory(field)
add_subdirectory(procs)
add_subdirectory(udm)

ug_add_dim_libs(np OBJECT SOURCES ${SOURCES})
#  OBJECT_DIM_LIBS algebra amg field procs udm)

install(FILES np.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
