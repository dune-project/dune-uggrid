find_package(X11)

# Compatibility against previous UG versions.
set(UG_USE_SYSTEM_HEAP ${UG_ENABLE_SYSTEM_HEAP})
set(UG_FOUND True)
set(HAVE_UG True)
set(UG_VERSION "${DUNE_UGGRID_VERSION}")
# The dune module is alway 3.13.0 or above!
add_definitions(-DUG_USE_NEW_DIMENSION_DEFINES)

if(UG_PARALLEL STREQUAL "yes")
  # Actually we probably should activate UG
  # for everything. But for the time being we fall
  # back to the enable trick. To change this just
  # uncomment the lines below.
  #add_definitions("-DENABLE_UG=1")
  #add_definitions("-DModelP")
  set(UG_DEFINITIONS "ENABLE_UG=1;ModelP")
else()
  # Actually we probably should activate UG
  # for everything. But for the time being we fall
  # back to the enable trick. To change this just
  # uncomment the lines below.
  #add_definitions("-DENABLE_UG=1")
  set(UG_DEFINITIONS "ENABLE_UG=1")
endif()

# Check whether dune-uggrid is installed
if(NOT dune-uggrid_INSTALLED)
  #Yes, then we have setup the complete include path to the ug headers.
  include_directories(${dune-uggrid_PREFIX} ${dune-uggrid_PREFIX}/low
    ${dune-uggrid_PREFIX}/gm ${dune-uggrid_PREFIX}/dev ${dune-uggrid_PREFIX}/dom
    ${dune-uggrid_PREFIX}/np ${dune-uggrid_PREFIX}/ui
    ${dune-uggrid_PREFIX}/graphics/uggraph ${dune-uggrid_PREFIX}/np/algebra
    ${dune-uggrid_PREFIX}/np/udm ${dune-uggrid_PREFIX}/np/procs
    ${dune-uggrid_PREFIX}/parallel ${dune-uggrid_PREFIX}/parallel/ddd
    ${dune-uggrid_PREFIX}/parallel/ppif ${dune-uggrid_PREFIX}/ddd/include
    ${dune-uggrid_PREFIX}/parallel/dddif ${dune-uggrid_PREFIX}/parallel/util
    ${dune-uggrid_PREFIX}/parallel/ddd/include )
endif(NOT dune-uggrid_INSTALLED)
