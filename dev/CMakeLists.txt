# The libs from these directories are needed for devices
#add_subdirectory(ppm)
#add_subdirectory(meta)
#add_subdirectory(ps)

set(SOURCES  ugdevices.cc sockcomm.cc
  ppm/ppm.cc meta/metafile.cc ps/postscript.cc ps/postscriptbw.cc)
set(devinclude_HEADERS  ugdevices.h)

add_library(devices OBJECT ${SOURCES})
# We need a unique name for the object library that will become libdevS
# later in lib
add_library(devSif OBJECT sif/sif.cc)
if(DUNE_BUILD_BOTH_LIBS)
  # For shared libraries we need position independent code
  set_property(TARGET devices devSif PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()
add_library(devR ${SOURCES} rif/remote.cc)
if(X11_FOUND)
  add_library(devX xif/xmain.cc xif/xgraph.cc xif/xshell.cc
    $<TARGET_OBJECTS:devices>)
  target_compile_definitions(devX PRIVATE "-DUG_DIM_2")
  target_link_libraries(devX "${X11_LIBRARIES}")
  install(TARGETS devX DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(X11_FOUND)
install(FILES ${devinclude_HEADERS} DESTINATION  ${CMAKE_INSTALL_PKGINCLUDEDIR})
