add_subdirectory(domain)
add_subdirectory(gm)
add_subdirectory(lib)
add_subdirectory(low)
add_subdirectory(parallel)

install(
  FILES ugdevices.h initug.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid)

add_library(devices OBJECT ugdevices.cc)
ug_add_dim_libs(ugui OBJECT ugdevices.cc)
