add_subdirectory(ugdata)

if(MPI_FOUND)
  set(_parallel parallel ddd analyser basic ctrl ident if join mgr prio xfer dddif)
endif()

set(_libs ug_gm gg np algebra amg field procs udm graphics uggrape uggraph ugui devices low)
ug_add_dim_libs(ugL SOURCES ../initug.cc OBJECT_DIM_LIBS ${_libs} domL ngin)
ug_add_dim_libs(ugS SOURCES ../initug.cc OBJECT_DIM_LIBS ${_libs} domS ${_parallel})
