# !!! properly integrate netgen
set(SUBDIRS  ngin ngin2d)
add_subdirectory(ngin)
add_subdirectory(ngin2d)
# !!! not good...
set(EXTRA_DIST  netgen/lgm_gginterface.h)
set(SOURCES  lgm_domain.cc lgm_load.cc lgm_parallel.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
ug_add_dim_libs(domL OBJECT SOURCES ${SOURCES}
  SOURCES_2D lgm_transfer2d.cc lgm_domain2d.cc
  SOURCES_3D lgm_transfer3d.cc lgm_domain3d.cc ansys2lgm.cc lgm_accel.cc)
#  OBJECT_DIM_LIBS ngin)

install(FILES lgm_domain.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
