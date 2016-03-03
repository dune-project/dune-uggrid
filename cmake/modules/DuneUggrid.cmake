find_package(Flex)
find_package(Bison)

if(MPI_FOUND)
  add_definitions("ModelP")
endif()

set(UG_ENABLE_SYSTEM_HEAP True CACHE BOOL
  "Whether to use the UG heap or the one of the operating system")

if(UG_ENABLE_SYSTEM_HEAP)
  set(UG_USE_SYSTEM_HEAP True)
