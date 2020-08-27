if(UG_ENABLE_PARALLEL)
  target_sources(duneuggrid PRIVATE ppif.cc)
endif()

target_sources(duneuggrid PRIVATE ppifcontext.cc)

install(FILES
  ppif.h
  ppifcontext.hh
  ppiftypes.hh
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ppif)
