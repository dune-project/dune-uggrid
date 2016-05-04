find_package(X11)

set(UG_ENABLE_SYSTEM_HEAP True CACHE BOOL
  "Whether to use the UG heap or the one of the operating system")

# Compatibility against previous UG versions.
set(UG_USE_SYSTEM_HEAP ${UG_ENABLE_SYSTEM_HEAP})
set(UG_FOUND True)
set(HAVE_UG True)
set(UG_VERSION "${DUNE_UGGRID_VERSION}")
add_definitions("-DENABLE_UG=1")
if(MPI_FOUND)
  add_definitions("-DModelP")
endif()

message("UG_USE_SYSTEM_HEAP=${UG_USE_SYSTEM_HEAP}")
message("UG_ENABLE_SYSTEM_HEAP=${UG_ENABLE_SYSTEM_HEAP}|")
