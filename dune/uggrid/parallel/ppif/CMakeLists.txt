if(UG_ENABLE_PARALLEL)
  target_sources(dune-uggrid PRIVATE ppif.cc)
endif()

target_sources(dune-uggrid PRIVATE ppifcontext.cc)

install(FILES
  ppif.h
  ppifcontext.hh
  ppiftypes.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ppif)
