add_subdirectory(dddif)
add_subdirectory(ddd)
add_subdirectory(util)
add_subdirectory(ppif)

if(MPI_FOUND)
 ug_add_dim_libs(parallel OBJECT initparallel.cc) # OBJECT_DIM_LIBS ddd dddif)
endif()
