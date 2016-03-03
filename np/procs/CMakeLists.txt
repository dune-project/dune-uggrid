set(SOURCES
  nls.cc
  ls.cc
  assemble.cc
  basics.cc
  blocking.cc
  iter.cc
  transfer.cc
  error.cc
  newton.cc
  ts.cc
  bdf.cc
  ew.cc
  ewn.cc
  amgtransfer.cc
  freebnd.cc
  db.cc
  fas.cc
  nliter.cc
  project.cc
  order.cc
  tstep.cc
  enewton.cc
  pstep.cc
  reinit.cc
  els.cc
  eiter.cc
  iter_2.cc)

set(npinclude_HEADERS  assemble.h transfer.h ts.h nls.h ls.h)

ug_add_dim_libs(procs OBJECT SOURCES ${SOURCES})

install(FILES ${npinclude_HEADERS} DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
