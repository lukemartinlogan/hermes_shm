cmake_minimum_required(VERSION 3.10)
project(hermes_shm)

set(CMAKE_CXX_STANDARD 17)

set(TEST_MAIN ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(data_structures)
add_subdirectory(allocators)
add_subdirectory(allocators_mpi)
add_subdirectory(backend)
add_subdirectory(types)
add_subdirectory(thread)

if (HERMES_ENABLE_COMPRESS)
    message("HERMES ENABLE COMPRESS")
    add_subdirectory(compress)
endif()

if (HERMES_ENABLE_ENCRYPT)
    message("HERMES ENABLE ENCRYPT")
    add_subdirectory(encrypt)
endif()