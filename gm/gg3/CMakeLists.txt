if(UG_ENABLE_3D)
  add_definitions("-DUG_DIM_3")
  add_library(gg3 OBJECT gg3d.cc)
  if(DUNE_BUILD_BOTH_LIBS)
    # For shared libraries we need position independent code
    set_property(TARGET gg3 PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  endif()
endif()
