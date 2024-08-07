cmake_minimum_required(VERSION 3.10)
project(hermes_shm)

set(CMAKE_CXX_STANDARD 17)

#------------------------------------------------------------------------------
# Global variables
#------------------------------------------------------------------------------
set(HERMES_SHM_VERSION_MAJOR 1)
set(HERMES_SHM_VERSION_MINOR 1)
set(HERMES_SHM_VERSION_PATCH 0)
set(HERMES_SHM_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${HERMES_SHM_ROOT}/include)

#------------------------------------------------------------------------------
# Options
#------------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(BUILD_HSHM_TESTS "Build tests" ON)
option(BUILD_HSHM_BENCHMARKS "Build benchmarks" ON)
option(BUILD_MPI_TESTS "Build tests which depend on MPI" ON)
option(BUILD_OpenMP_TESTS "Build tests which depend on OpenMP" ON)
option(BUILD_Boost_TESTS "Build tests which depend on libboost" ON)
option(HERMES_RPC_THALLIUM "Build tests which depend on thallium" ON)
option(HERMES_ENABLE_CEREAL "Enable serialization using cereal" ON)
option(HERMES_ENABLE_COVERAGE "Check how well tests cover code" OFF)
option(HERMES_ENABLE_DOXYGEN "Check how well the code is documented" ON)
option(HERMES_CXX_PROFILE "Generate profiling data from benchmarks" OFF)
option(HERMES_PTHREADS_ENABLED "Support spawning pthreads" ON)
option(HERMES_DEBUG_LOCK "Used for debugging locks" OFF)
option(HERMES_ENABLE_PROFILING "View profiling logs" OFF)
option(HERMES_ENABLE_COMPRESS "Enable compression" OFF)
option(HERMES_ENABLE_ENCRYPT "Enable encryption" OFF)
option(HERMES_USE_ELF "Enable encryption" ON)

if (HERMES_PTHREADS_ENABLED)
    add_compile_definitions(HERMES_PTHREADS_ENABLED)
endif()
if (HERMES_RPC_THALLIUM)
    add_compile_definitions(HERMES_RPC_THALLIUM)
endif()
if (HERMES_DEBUG_LOCK)
    add_compile_definitions(HERMES_DEBUG_LOCK)
endif()

#------------------------------------------------------------------------------
# Setup CMake Environment
#------------------------------------------------------------------------------
if(NOT HERMES_EXTERNALLY_CONFIGURED)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries.")
endif()

#------------------------------------------------------------------------------
# Setup install and output Directories
#------------------------------------------------------------------------------
if(NOT HERMES_INSTALL_BIN_DIR)
    set(HERMES_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()
if(NOT HERMES_INSTALL_LIB_DIR)
    set(HERMES_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()
if(NOT HERMES_INSTALL_INCLUDE_DIR)
    set(HERMES_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()
if(NOT HERMES_INSTALL_DATA_DIR)
    set(HERMES_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

#------------------------------------------------------------------------------
# CMake Modules
#------------------------------------------------------------------------------
if (NOT IS_HERMES_MAIN)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
    message(${CMAKE_MODULE_PATH})
endif()

#------------------------------------------------------------------------------
# Optimization
#------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("IN DEBUG MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
else()
    message("IN RELEASE MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
endif()
if(HERMES_CXX_PROFILE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    message("CXX PROFILER IS ON")
endif()
#function(make_gprof exec_name exec_dir)
#    add_custom_target(
#      ${exec_name}_gprof
#      COMMAND gprof -b -A -p -q ${exec_dir}/${exec_name} gmon.out)
#endfunction()

#------------------------------------------------------------------------------
# External libraries
#------------------------------------------------------------------------------

# Catch2
find_package(Catch2 3.0.1 REQUIRED)
# find_package(Catch2 REQUIRED)
message(STATUS "found catch2.h at ${Catch2_CXX_INCLUDE_DIRS}")

# MPICH
if(BUILD_MPI_TESTS)
    find_package(MPI REQUIRED COMPONENTS C CXX)
    message(STATUS "found mpi.h at ${MPI_CXX_INCLUDE_DIRS}")
endif()

# OpenMP
if(BUILD_OpenMP_TESTS)
    find_package(OpenMP REQUIRED COMPONENTS C CXX)
    message(STATUS "found omp.h at ${OpenMP_CXX_INCLUDE_DIRS}")
endif()

# thallium
if(HERMES_RPC_THALLIUM)
    find_package(thallium CONFIG REQUIRED)
    if(thallium_FOUND)
        message(STATUS "found thallium at ${thallium_DIR}")
    endif()
endif()

# Cereal
if(HERMES_ENABLE_CEREAL)
    find_package(cereal CONFIG REQUIRED)
    if(cereal_FOUND)
        message(STATUS "found cereal at ${cereal_DIR}")
    endif()
endif()

# Boost
if(BUILD_Boost_TESTS)
    find_package(Boost REQUIRED)
    message(STATUS "found boost.h at ${Boost_INCLUDE_DIRS}")
endif()

# Compression libraries
if(HERMES_ENABLE_COMPRESS)
    pkg_check_modules(bzip2 REQUIRED bzip2)
    message(STATUS "found bz2.h at ${bzip2_INCLUDE_DIRS}")

    pkg_check_modules(lzo2 REQUIRED lzo2)
    message(STATUS "found lzo2.h at ${lzo2_INCLUDE_DIRS}")
    get_filename_component(lzo2_dir "${lzo2_INCLUDE_DIRS}" DIRECTORY)

    pkg_check_modules(libzstd REQUIRED libzstd)
    message(STATUS "found zstd.h at ${libzstd_INCLUDE_DIRS}")

    pkg_check_modules(liblz4 REQUIRED liblz4)
    message(STATUS "found lz4.h at ${liblz4_INCLUDE_DIRS}")

    pkg_check_modules(zlib REQUIRED zlib)
    message(STATUS "found zlib.h at ${zlib_INCLUDE_DIRS}")

    pkg_check_modules(liblzma REQUIRED liblzma)
    message(STATUS "found liblzma.h at ${liblzma_INCLUDE_DIRS}")

    pkg_check_modules(libbrotlicommon REQUIRED libbrotlicommon libbrotlidec libbrotlienc)
    message(STATUS "found libbrotli.h at ${libbrotlicommon_INCLUDE_DIRS}")

    pkg_check_modules(snappy REQUIRED snappy)
    message(STATUS "found libbrotli.h at ${snappy_INCLUDE_DIRS}")

    pkg_check_modules(blosc2 REQUIRED blosc2)
    message(STATUS "found blosc2.h at ${blosc2_INCLUDE_DIRS}")

    set(COMPRESS_LIBS
            bz2
            ${lzo2_LIBRARIES}
            ${libzstd_LIBRARIES}
            ${liblz4_LIBRARIES}
            ${zlib_LIBRARIES}
            ${liblzma_LIBRARIES}
            ${libbrotlicommon_LIBRARIES}
            ${snappy_LIBRARIES}
            ${blosc2_LIBRARIES}
    )
    include_directories(
            ${bzip2_INCLUDE_DIRS}
            ${lzo2_INCLUDE_DIRS} ${lzo2_dir}
            ${libzstd_INCLUDE_DIRS}
            ${liblz4_INCLUDE_DIRS}
            ${zlib_INCLUDE_DIRS}
            ${liblzma_INCLUDE_DIRS}
            ${libbrotlicommon_INCLUDE_DIRS}
            ${snappy_INCLUDE_DIRS}
            ${blosc2_INCLUDE_DIRS}
    )
    link_directories(
            ${bzip2_LIBRARY_DIRS}
            ${lzo2_LIBRARY_DIRS}
            ${libzstd_LIBRARY_DIRS}
            ${liblz4_LIBRARY_DIRS}
            ${zlib_LIBRARY_DIRS}
            ${liblzma_LIBRARY_DIRS}
            ${libbrotlicommon_LIBRARY_DIRS}
            ${snappy_LIBRARY_DIRS}
            ${blosc2_LIBRARY_DIRS}
    )
endif()

# Encryption libraries
if(HERMES_ENABLE_ENCRYPT)
    pkg_check_modules(libcrypto REQUIRED libcrypto)
    message(STATUS "found libcrypto.h at ${libcrypto_INCLUDE_DIRS}")

    set(ENCRYPT_LIBS ${libcrypto_LIBRARIES})
    include_directories(${libcrypto_INCLUDE_DIRS})
    link_directories(${libcrypto_LIBRARY_DIRS})
endif()

#-----------------------------------------------------------------------------
# Documentation
#-----------------------------------------------------------------------------
if(HERMES_ENABLE_DOXYGEN)
    include(UseDoxygenDoc)

    add_doxygen_doc(
            BUILD_DIR
            ${CMAKE_CURRENT_BINARY_DIR}/_build
            DOXY_FILE
            ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
            TARGET_NAME
            dox
            COMMENT
            "HTML documentation"
    )
endif()

#------------------------------------------------------------------------------
# Code Coverage
#------------------------------------------------------------------------------
if (NOT IS_HERMES_MAIN)
    if(HERMES_ENABLE_COVERAGE)
        set(COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage" CACHE STRING
            "Flags to the coverage program to perform coverage inspection")
        mark_as_advanced(COVERAGE_FLAGS)

        macro(set_coverage_flags target)
            set_target_properties(${target}
                    PROPERTIES
                    COMPILE_FLAGS ${COVERAGE_FLAGS}
                    LINK_FLAGS ${COVERAGE_FLAGS}
                    )
        endmacro()
    endif()
endif()

#------------------------------------------------------------------------------
# Build hermes_shm
#------------------------------------------------------------------------------
add_subdirectory(src)

if (NOT IS_HERMES_MAIN)
    add_custom_target(lint COMMAND bash ${HERMES_SHM_ROOT}/scripts/lint.sh ${HERMES_SHM_ROOT})
    add_custom_target(preamble COMMAND python3 ${HERMES_SHM_ROOT}/scripts/preamble.py ${HERMES_SHM_ROOT})
endif()

#------------------------------------------------------------------------------
# Build tests + benchmarks
#------------------------------------------------------------------------------
set(TEST_MAIN ${HERMES_SHM_ROOT}/test/unit)
enable_testing()
if (BUILD_HSHM_TESTS)
  message("Building HSHM unit tests")
  add_subdirectory(test)
endif()
if (BUILD_HSHM_BENCHMARKS)
  message("Building HSHM benchmarks")
  add_subdirectory(benchmark)
endif()
#add_subdirectory(example)

#------------------------------------------------------------------------------
# Install hshm
#------------------------------------------------------------------------------
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/HermesShmConfig.cmake
        ${PROJECT_BINARY_DIR}/CMakeFiles/HermesShmConfig.cmake @ONLY
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/UseDoxygenDoc.cmake
        ${PROJECT_BINARY_DIR}/CMakeFiles/UseDoxygenDoc.cmake @ONLY
)

install(
    FILES
        ${PROJECT_BINARY_DIR}/CMakeFiles/HermesShmConfig.cmake
        ${PROJECT_BINARY_DIR}/CMakeFiles/UseDoxygenDoc.cmake
    DESTINATION
        ${CMAKE_INSTALL_PREFIX}/cmake
)
