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
add_library(devSo OBJECT $<TARGET_OBJECTS:devices> sif/sif.cc)
add_library(devR ${SOURCES} rif/remote.cc)
add_library(devX xif/xmain.cc xif/xgraph.cc xif/xshell.cc
  $<TARGET_OBJECTS:devices>)
target_compile_definitions(devX PRIVATE "-DUG_DIM_2")
target_link_libraries(devX "${X11_LIBRARIES}")
#add_subdirectory(sif)
#add_subdirectory(xif)
#add_subdirectory(rif)

install(TARGETS devX DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${devinclude_HEADERS} DESTINATION  ${CMAKE_INSTALL_PKGINCLUDEDIR})
