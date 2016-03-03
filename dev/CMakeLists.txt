# The libs from these directories are needed for devices
#add_subdirectory(ppm)
#add_subdirectory(meta)
#add_subdirectory(ps)

set(SOURCES  ugdevices.cc sockcomm.cc
  ppm/ppm.cc meta/metafile.cc ps/postscript.cc ps/postscriptbw.cc)
set(devincludedir  ${CMAKE_INSTALL_INCLUDEDIR})
set(devinclude_HEADERS  ugdevices.h)

ug_add_dim_libs(devices OBJECT ${SOURCES})
add_library(devS ${SOURCES} sif/sif.cc)
add_library(devR ${SOURCES} rif/remote.cc)
add_library(devX ${SOURCE} xif/xmain.cc xif/xgraph.cc xif/xshell.cc)
target_compile_definitions(devX PRIVATE "-D_2")
#add_subdirectory(sif)
#add_subdirectory(xif)
#add_subdirectory(rif)

install(FILES ${devinclude_HEADERS} DESTINATION  ${CMAKE_INSTALL_PKGINCLUDEDIR})
