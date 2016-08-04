set(SOURCES initui.cc cmdline.cc
  commands.cc
)
install(FILES cmdint.h cmdline.h commands.h
  DESTINATION ${CMAKE_INSTALL_PKGINCLUDEDIR})
ug_add_dim_libs(ugui OBJECT SOURCES ${SOURCES})
