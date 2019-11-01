#ifndef RM_WRITE2FILE_H
#define RM_WRITE2FILE_H

// standard C library
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstdlib>

// std library
#include <stdio.h>
#include <string>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>
#include <iterator>

#include <dev/ugdevices.h>
#include "gm.h"
#include "rm.h"
#include "evm.h"

#define TET_RULE_FATHER_SIDE_OFFSET 20

int WriteSonData(std::FILE* stream, NS_DIM_PREFIX sondata const& son);
void WriteRule2File(std::FILE* stream, NS_DIM_PREFIX REFRULE const& rules);
void Write2File(std::FILE* stream, std::vector<NS_DIM_PREFIX REFRULE> const& rules, std::vector<NS_PREFIX SHORT> const& patterns);

START_UGDIM_NAMESPACE
void readTetrahedronRules(FILE* stream, std::vector<NS_DIM_PREFIX REFRULE>& rules, std::vector<NS_PREFIX SHORT>& patterns);
END_UGDIM_NAMESPACE

#endif
