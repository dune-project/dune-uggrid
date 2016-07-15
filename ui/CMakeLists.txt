set(SOURCES  uginterface.cc initui.cc cmdline.cc
  commands.cc helpmsg.cc
  mmio.cc fieldio.cc)
install(FILES cmdint.h cmdline.h commands.h
  DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
ug_add_dim_libs(ugui OBJECT SOURCES ${SOURCES})
