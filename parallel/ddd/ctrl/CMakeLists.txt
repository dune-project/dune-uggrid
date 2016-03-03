add_definitions(ConsMemFromHeap)

if(MPI_FOUND)
  ug_add_dim_libs(ctrl OBJECT SOURCES cons.cc debug.cc stat.cc)
endif()
install(FILES stat.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
