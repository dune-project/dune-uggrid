# generate header file, rename prefix
bison_target(nginyacc ngin-yacc.yy ${CMAKE_CURRENT_BINARY_DIR}/ngic-yac.cc
  COMPILE_FLAGS "-d -p ${PARSERPREFIX}")
flex_target(nginlex ngin-lex.ll ${CMAKE_CURRENT_BINARY_DIR}/ngin-lex.cc
  COMPILE_FLAGS "-P${PARSERPREFIX}")

add_library(ngin3 OBJECT ${BISON_nginyacc_OUTPUTS} ${FLEX_nginlex_OUTPUT})
