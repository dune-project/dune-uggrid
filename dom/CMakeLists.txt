# compile the different domain-modules
add_subdirectory(std)
add_subdirectory(lgm)
# does not compile with C++-compiler
# gen
# install the central header
set(domincludedir  ${CMAKE_INSTALL_INCLUDEDIR})
set(dominclude_HEADERS  domain.h)
install(FILES ${dominclude_HEADERS} DESTINATION ${domincludedir})
