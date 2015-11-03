################################################################################
# created by Stephan Grein <stephan.grein@gcsc.uni-frankfurt.de>
# derived from juqueen.cmake established by Ingo Heppner.
#
# enables OpenMP for IBM XL compiler and associated tools on JUQUEEN (BlueGene/Q)
################################################################################

# set CMAKE_SYSTEM_NAME to include automatically corresponding platform files
set(CMAKE_SYSTEM_NAME BlueGeneQ-static)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# no dynamic build
SET(enableDynamicOption OFF)

# add -fno-strict-aliasing option to the compiler flags
SET(enableNoStrictAliasingOption ON)

# variables (note _r suffix indicates OpenMP compiler)
set(GCC_ROOT  "/bgsys/drivers/ppcfloor/gnu-linux")
set(GCC_NAME  "powerpc64-bgq-linux")
set(MPI_ROOT  "/bgsys/drivers/ppcfloor/comm/gcc")

# serial GCC
set(CMAKE_C_COMPILER       ${GCC_ROOT}/bin/${GCC_NAME}-gcc)
set(CMAKE_CXX_COMPILER     ${GCC_ROOT}/bin/${GCC_NAME}-g++)
set(CMAKE_Fortran_COMPILER ${GCC_ROOT}/bin/${GCC_NAME}-gfortran)

# mpi wrappers for GCC (note _r suffix indicates OpenMP compiler)
set(MPI_C_COMPILER       ${MPI_ROOT}/bin/mpicc)
set(MPI_CXX_COMPILER     ${MPI_ROOT}/bin/mpicxx)
set(MPI_Fortran_COMPILER ${MPI_ROOT}/bin/mpif90)

message(STATUS "TMP INFO: Value of '\${CMAKE_C_COMPILER}'       is: ${CMAKE_C_COMPILER}")       # TMP
message(STATUS "TMP INFO: Value of '\${CMAKE_CXX_COMPILER}'     is: ${CMAKE_CXX_COMPILER}")     # TMP
message(STATUS "TMP INFO: Value of '\${CMAKE_Fortran_COMPILER}' is: ${CMAKE_Fortran_COMPILER}") # TMP

message(STATUS "TMP INFO: Value of '\${MPI_C_COMPILER}'         is: ${MPI_C_COMPILER}")         # TMP
message(STATUS "TMP INFO: Value of '\${MPI_CXX_COMPILER}'       is: ${MPI_CXX_COMPILER}")       # TMP
message(STATUS "TMP INFO: Value of '\${MPI_Fortran_COMPILER}'   is: ${MPI_Fortran_COMPILER}")   # TMP

# add compiler flag -fopenmp (OpenMP)
SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fopenmp")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fopenmp")

# XL specific options (not known by GCC to my knowledge)
SET( CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -qsmp=omp -qnosave")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -qsmp=omp -qnosave")