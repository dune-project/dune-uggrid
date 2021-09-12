# Compatibility against previous UG versions.
set(UG_FOUND True)
set(HAVE_UG True)
set(UG_VERSION "${DUNE_UGGRID_VERSION}")

set(UG_DEFINITIONS)
if(UG_PARALLEL)
  list(APPEND UG_DEFINITIONS "ModelP")
endif()