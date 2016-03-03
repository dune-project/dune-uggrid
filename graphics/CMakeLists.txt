add_subdirectory(uggraph)
add_subdirectory(grape)
set(SOURCES  graphics.cc)# graphics.h)

ug_add_dim_libs(graphics OBJECT SOURCES ${SOURCES})
#  OBJECT_DIM_LIBS uggraph grape)

install(FILES graphics.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
