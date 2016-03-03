if(MPI_FOUND)
  ug_add_dim_libs(ident OBJECT SOURCES ident.cc)
endif()

install(FILES stat.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
