if(UG_ENABLE_3D)
  add_definitions("-DUG_DIM_3")
  add_library(gg3 OBJECT gg3d.cc)
endif()
