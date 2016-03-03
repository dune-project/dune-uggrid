set(SOURCES  cmds.cc
  cmdmsg.cc
  cplmsg.cc
  ctrl.cc
  pack.cc
  supp.cc
  unpack.cc
  xfer.cc)

if(MPI_FOUND)
  ug_add_dim_libs(xfer OBJECT SOURCES ${SOURCES})
endif()
install(FILES sll.h xfer.h  DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
