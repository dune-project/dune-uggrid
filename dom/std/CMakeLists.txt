set(SOURCES  std_domain.cc std_parallel.cc) # std_domain.h std_internal.h)
ug_add_dim_libs(domS OBJECT SOURCES ${SOURCES} SOURCES_2D domains2d.cc
  SOURCES_3D domains3d.cc)
install(FILES std_domain.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
