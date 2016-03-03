if(MPI_FOUND)
  add_definitions("-DModelP")
endif()

set(UG_ENABLE_SYSTEM_HEAP True CACHE BOOL
  "Whether to use the UG heap or the one of the operating system")

set(UG_USE_SYSTEM_HEAP ${UG_ENABLE_SYSTEM_HEAP})

message("UG_USE_SYSTEM_HEAP=${UG_USE_SYSTEM_HEAP}")
message("UG_ENABLE_SYSTEM_HEAP=${UG_ENABLE_SYSTEM_HEAP}|")
