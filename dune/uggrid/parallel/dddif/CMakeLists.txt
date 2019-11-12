install(FILES parallel.h DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})

if(MPI_C_FOUND)
  include_directories(${PROJECT_SOURCE_DIR}/dune/uggrid/parallel/ddd/include)
  ug_add_dim_libs(dddif OBJECT
    SOURCES
    compat.cc
    debugger.cc
    gridcons.cc
    handler.cc
    identify.cc
    identify.h
    initddd.cc
    lb.cc
    lbrcb.cc
    memmgr.cc
    overlap.cc
    priority.cc
    pgmcheck.cc
    partition.cc
    support.cc
    trans.cc
    )
endif()
