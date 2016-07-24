# The libs from these directories are needed for devices
#add_subdirectory(meta)
#add_subdirectory(ps)

set(SOURCES  ugdevices.cc sockcomm.cc
  meta/metafile.cc)
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
install(FILES ${devinclude_HEADERS} DESTINATION  ${CMAKE_INSTALL_PKGINCLUDEDIR})
