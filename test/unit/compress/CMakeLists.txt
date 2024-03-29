cmake_minimum_required(VERSION 3.10)
project(hermes_shm)

set(CMAKE_CXX_STANDARD 17)

#------------------------------------------------------------------------------
# Build Tests
#------------------------------------------------------------------------------
add_executable(test_compress_exec
        ${TEST_MAIN}/main.cc
        test_init.cc
        test_compress.cc)
add_dependencies(test_compress_exec hermes_shm_data_structures)
target_link_libraries(test_compress_exec
        hermes_shm_data_structures Catch2::Catch2
        MPI::MPI_CXX OpenMP::OpenMP_CXX)

add_test(NAME test_compress COMMAND
        ${CMAKE_BINARY_DIR}/bin/test_compress_exec "~[error=FatalError]")

#-----------------------------------------------------------------------------
# Install Targets
#------------------------------------------------------------------------------
install(TARGETS
        test_compress_exec
        EXPORT
        ${HERMES_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${HERMES_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${HERMES_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${HERMES_INSTALL_BIN_DIR})

#-----------------------------------------------------------------------------
# Coverage
#-----------------------------------------------------------------------------
if(HERMES_ENABLE_COVERAGE)
    set_coverage_flags(test_compress_exec)
endif()