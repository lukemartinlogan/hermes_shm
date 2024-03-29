/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#ifndef HERMES_SHM_CONTAINER_H_
#define HERMES_SHM_CONTAINER_H_

#include "hermes_shm/memory/memory_registry.h"
#include "hermes_shm/constants/macros.h"
#include "shm_container_macro.h"
#include "shm_macros.h"

namespace hipc = hshm::ipc;

namespace hshm::ipc {

/** The shared-memory header used for data structures */
template<typename T>
struct ShmHeader;

/** The ShmHeader used for base containers */
#define SHM_CONTAINER_HEADER_TEMPLATE(HEADER_NAME)\
  /** Default constructor */\
  TYPE_UNWRAP(HEADER_NAME)() = default;\
  \
  /** Copy constructor */\
  TYPE_UNWRAP(HEADER_NAME)(const TYPE_UNWRAP(HEADER_NAME) &other) {\
    strong_copy(other);\
  }\
  \
  /** Copy assignment operator */\
  TYPE_UNWRAP(HEADER_NAME)& operator=(const TYPE_UNWRAP(HEADER_NAME) &other) {\
    if (this != &other) {\
      strong_copy(other);\
    }\
    return *this;\
  }\
\
  /** Move constructor */\
  TYPE_UNWRAP(HEADER_NAME)(TYPE_UNWRAP(HEADER_NAME) &&other) {\
    strong_copy(other);\
  }\
  \
  /** Move operator */\
  TYPE_UNWRAP(HEADER_NAME)& operator=(TYPE_UNWRAP(HEADER_NAME) &&other) {\
    if (this != &other) {\
      strong_copy(other);\
    }\
    return *this;\
  }

/** The ShmHeader used for wrapper containers */
struct ShmWrapperHeader {};

/**
 * ShmContainers all have a header, which is stored in
 * shared memory as a TypedPointer.
 * */
class ShmContainer {};

/** Typed nullptr */
template<typename T>
HSHM_ALWAYS_INLINE static T* typed_nullptr() {
  return reinterpret_cast<T*>(NULL);
}

}  // namespace hshm::ipc

#endif  // HERMES_SHM_CONTAINER_H_
