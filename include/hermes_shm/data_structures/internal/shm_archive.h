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


#ifndef HERMES_DATA_STRUCTURES_SHM_ARCHIVE_H_
#define HERMES_DATA_STRUCTURES_SHM_ARCHIVE_H_

#include "shm_macros.h"
#include "hermes_shm/memory/memory.h"
#include "shm_deserialize.h"

namespace hermes_shm::ipc {

/**
 * Indicates that a data structure can be archived in shared memory
 * and has a corresponding TypedPointer override.
 * */
class ShmArchiveable {};

/**
 * Constructs a TypedPointer in-place
 * */
template<typename T>
class _ShmArchive_Header {
 public:
  typedef typename T::header_t header_t;
  char obj_hdr_[sizeof(header_t)];

 public:
  /** Default constructor */
  _ShmArchive_Header() = default;

  /** Destructor */
  ~_ShmArchive_Header() = default;

  /** SHM Construct + store object */
  template<typename ...Args>
  void shm_init(Allocator *alloc, Args&& ...args) {
    T(reinterpret_cast<header_t&>(obj_hdr_),
      alloc, std::forward<Args>(args)...).UnsetDestructable();
  }

  /** SHM Construct + store object (hermes rval argpack) */
  template<typename ArgPackT>
  void shm_init_piecewise(Allocator *alloc, ArgPackT &&args) {
    T obj;
    PassArgPack::Call(
      MergeArgPacks::Merge(make_argpack(obj,
                                        reinterpret_cast<header_t&>(obj_hdr_),
                                        alloc),
                           std::forward<ArgPackT>(args)),
      [](auto&& ...Args) constexpr {
        Allocator::ConstructObj<T>(std::forward<decltype(Args)>(Args)...);
      });
    obj.UnsetDestructable();
  }

  /** SHM destructor */
  void shm_destroy(Allocator *alloc) {
    auto ar = internal_ref(alloc);
    T obj;
    obj.shm_deserialize(ar);
    obj.shm_destroy();
  }

  /** Returns a reference to the internal object */
  ShmDeserialize<T> internal_ref(Allocator *alloc) {
    return ShmDeserialize<T>(reinterpret_cast<header_t*>(obj_hdr_), alloc);
  }

  /** Returns a reference to the internal object */
  ShmDeserialize<T> internal_ref(Allocator *alloc) const {
    return ShmDeserialize<T>(reinterpret_cast<header_t*>(obj_hdr_), alloc);
  }
};

/**
 * Constructs an object in-place
 * */
template<typename T>
class _ShmArchive_T {
 public:
  char obj_[sizeof(T)]; /**< Store object without constructing */

 public:
  /** Default constructor. */
  _ShmArchive_T() = default;

  /** Destructor. Does nothing. */
  ~_ShmArchive_T() = default;

  /** Construct + store object (C++ argpack) */
  template<typename ...Args>
  void shm_init(Allocator *alloc, Args&& ...args) {
    Allocator::ConstructObj<T>(
      internal_ref(alloc), std::forward<Args>(args)...);
  }

  /** Construct + store object (hermes rval argpack) */
  template<typename ArgPackT>
  void shm_init_piecewise(Allocator *alloc, ArgPackT &&args) {
    hermes_shm::PassArgPack::Call(
      MergeArgPacks::Merge(
        make_argpack(internal_ref(alloc)),
        std::forward<ArgPackT>(args)),
      [](auto&& ...Args) constexpr {
        Allocator::ConstructObj<T>(std::forward<decltype(Args)>(Args)...);
      });
  }

  /** Shm destructor */
  void shm_destroy(Allocator *alloc) {
    Allocator::DestructObj<T>(internal_ref(alloc));
  }

  /** Returns a reference to the internal object */
  T& internal_ref(Allocator *alloc) {
    (void) alloc;
    return reinterpret_cast<T&>(obj_);
  }

  /** Returns a reference to the internal object */
  T& internal_ref(Allocator *alloc) const {
    (void) alloc;
    return reinterpret_cast<T&>(obj_);
  }
};

/**
 * Whether or not to use _ShmArchive or _ShmArchive_T
 * */
#define SHM_MAKE_HEADER_OR_T(T) \
  SHM_X_OR_Y(T, _ShmArchive_Header<T>, _ShmArchive_T<T>)

/**
 * Used for data structures which intend to store:
 * 1. An archive if the data type is SHM_ARCHIVEABLE
 * 2. The raw type if the data type is anything else
 *
 * E.g., used in unordered_map for storing collision entries.
 * E.g., used in a list for storing list entries.
 * */
template<typename T>
class ShmArchive {
 public:
  typedef SHM_ARCHIVE_OR_REF(T) T_Ar;
  typedef SHM_MAKE_HEADER_OR_T(T) T_Hdr;
  T_Hdr obj_;

  /** Default constructor */
  ShmArchive() = default;

  /** Construct object */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    obj_.shm_init(std::forward<Args>(args)...);
  }

  /** Construct + store object (hermes rval argpack) */
  template<typename ArgPackT>
  void shm_init_piecewise(Allocator *alloc, ArgPackT &&args) {
    obj_.shm_init_piecewise(alloc, std::forward<ArgPackT>(args));
  }

  /** Destructor */
  ~ShmArchive() = default;

  /** Returns a reference to the internal object */
  T_Ar internal_ref(Allocator *alloc) {
    return obj_.internal_ref(alloc);
  }

  /** Returns a reference to the internal object */
  T_Ar internal_ref(Allocator *alloc) const {
    return obj_.internal_ref(alloc);
  }

  /** Shm destructor */
  void shm_destroy(Allocator *alloc) {
    obj_.shm_destroy(alloc);
  }

  /** Copy constructor */
  ShmArchive(const ShmArchive &other) = delete;

  /** Copy assignment operator */
  ShmArchive& operator=(const ShmArchive &other) = delete;

  /** Move constructor */
  ShmArchive(ShmArchive &&other) = delete;

  /** Move assignment operator */
  ShmArchive& operator=(ShmArchive &&other) = delete;
};

}  // namespace hermes_shm::ipc

#endif  // HERMES_DATA_STRUCTURES_SHM_ARCHIVE_H_
