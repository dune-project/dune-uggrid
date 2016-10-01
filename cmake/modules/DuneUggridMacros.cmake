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
