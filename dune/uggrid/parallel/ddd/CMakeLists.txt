install(FILES
  dddconstants.hh
  dddcontext.hh
  dddtypes.hh
  dddtypes_impl.hh
  include/ddd.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd)

if(UG_ENABLE_PARALLEL)
  add_subdirectory(analyser)
  add_subdirectory(basic)
  add_subdirectory(ctrl)
  add_subdirectory(ident)
  add_subdirectory(if)
  add_subdirectory(join)
  add_subdirectory(mgr)
  add_subdirectory(prio)
  add_subdirectory(xfer)

  if(HAVE_MPI)
    ug_add_dim_libs(ddd OBJECT ddd.cc)
  endif()
endif()
