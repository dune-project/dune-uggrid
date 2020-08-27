if(MPI_C_FOUND)
  target_sources_dims(duneuggrid PRIVATE
    cmds.cc
    cmdmsg.cc
    cplmsg.cc
    ctrl.cc
    pack.cc
    supp.cc
    unpack.cc
    xfer.cc)
endif()

install(FILES
  sll.h xfer.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/uggrid/parallel/ddd/xfer)
