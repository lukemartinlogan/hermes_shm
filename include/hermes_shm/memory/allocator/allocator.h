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

#ifndef HERMES_MEMORY_ALLOCATOR_ALLOCATOR_H_
#define HERMES_MEMORY_ALLOCATOR_ALLOCATOR_H_

#include <cstdint>
#include <hermes_shm/memory/memory.h>
#include <hermes_shm/util/errors.h>

namespace hshm::ipc {

/**
 * The allocator type.
 * Used to reconstruct allocator from shared memory
 * */
enum class AllocatorType {
  kStackAllocator,
  kMallocAllocator,
  kFixedPageAllocator,
  kScalablePageAllocator,
};

/**
 * The basic shared-memory allocator header.
 * Allocators inherit from this.
 * */
struct AllocatorHeader {
  int allocator_type_;
  allocator_id_t allocator_id_;
  size_t custom_header_size_;

  AllocatorHeader() = default;

  void Configure(allocator_id_t allocator_id,
                 AllocatorType type,
                 size_t custom_header_size) {
    allocator_type_ = static_cast<int>(type);
    allocator_id_ = allocator_id;
    custom_header_size_ = custom_header_size;
  }
};

/**
 * The allocator base class.
 * */
class Allocator {
 protected:
  char *buffer_;
  size_t buffer_size_;
  char *custom_header_;

 public:
  /**
   * Constructor
   * */
  Allocator() : custom_header_(nullptr) {}

  /**
   * Destructor
   * */
  virtual ~Allocator() = default;

  /**
   * Create the shared-memory allocator with \a id unique allocator id over
   * the particular slot of a memory backend.
   *
   * The shm_init function is required, but cannot be marked virtual as
   * each allocator has its own arguments to this method. Though each
   * allocator must have "id" as its first argument.
   * */
  // virtual void shm_init(allocator_id_t id, Args ...args) = 0;

  /**
   * Deserialize allocator from a buffer.
   * */
  virtual void shm_deserialize(char *buffer,
                               size_t buffer_size) = 0;

  /**
   * Allocate a region of memory of \a size size
   * */
  virtual OffsetPointer AllocateOffset(size_t size) = 0;

  /**
   * Allocate a region of memory to a specific pointer type
   * */
  template<typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE PointerT Allocate(size_t size) {
    return PointerT(GetId(), AllocateOffset(size).load());
  }

  /**
   * Allocate a region of memory of \a size size
   * and \a alignment alignment. Assumes that
   * alignment is not 0.
   * */
  virtual OffsetPointer AlignedAllocateOffset(size_t size,
                                              size_t alignment) = 0;

  /**
   * Allocate a region of memory to a specific pointer type
   * */
  template<typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE PointerT AlignedAllocate(size_t size, size_t alignment) {
    return PointerT(GetId(), AlignedAllocateOffset(size, alignment).load());
  }

  /**
   * Allocate a region of \a size size and \a alignment
   * alignment. Will fall back to regular Allocate if
   * alignmnet is 0.
   * */
  template<typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE  PointerT Allocate(size_t size, size_t alignment) {
    if (alignment == 0) {
      return Allocate<PointerT>(size);
    } else {
      return AlignedAllocate<PointerT>(size, alignment);
    }
  }

  /**
   * Reallocate \a pointer to \a new_size new size
   * If p is kNullPointer, will internally call Allocate.
   *
   * @return true if p was modified.
   * */
  template<typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE bool Reallocate(PointerT &p, size_t new_size) {
    if (p.IsNull()) {
      p = Allocate<PointerT>(new_size);
      return true;
    }
    auto new_p = ReallocateOffsetNoNullCheck(p.ToOffsetPointer(),
                                             new_size);
    bool ret = new_p == p.ToOffsetPointer();
    p.off_ = new_p.load();
    return ret;
  }

  /**
   * Reallocate \a pointer to \a new_size new size.
   * Assumes that p is not kNullPointer.
   *
   * @return true if p was modified.
   * */
  virtual OffsetPointer ReallocateOffsetNoNullCheck(OffsetPointer p,
                                                    size_t new_size) = 0;

  /**
   * Free the memory pointed to by \a p Pointer
   * */
  template<typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE void Free(PointerT &p) {
    if (p.IsNull()) {
      throw INVALID_FREE.format();
    }
    FreeOffsetNoNullCheck(OffsetPointer(p.off_.load()));
  }

  /**
   * Free the memory pointed to by \a ptr Pointer
   * */
  virtual void FreeOffsetNoNullCheck(OffsetPointer p) = 0;

  /**
   * Get the allocator identifier
   * */
  virtual allocator_id_t &GetId() = 0;

  /**
   * Get the amount of memory that was allocated, but not yet freed.
   * Useful for memory leak checks.
   * */
  virtual size_t GetCurrentlyAllocatedSize() = 0;

  /**====================================
  * Pointer Allocators
  * ===================================*/

  /**
   * Allocate a pointer of \a size size and return \a p process-independent
   * pointer and a process-specific pointer.
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* AllocatePtr(size_t size,
                                    PointerT &p, size_t alignment = 0) {
    p = Allocate<PointerT>(size, alignment);
    if (p.IsNull()) { return nullptr; }
    return reinterpret_cast<T*>(buffer_ + p.off_.load());
  }

  /**
   * Allocate a pointer of \a size size
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* AllocatePtr(size_t size, size_t alignment = 0) {
    PointerT p;
    return AllocatePtr<T, PointerT>(size, p, alignment);
  }

  /**
   * Allocate a pointer of \a size size and return \a p process-independent
   * pointer and a process-specific pointer.
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  LPointer<T, PointerT>
  AllocateLocalPtr(size_t size, size_t alignment = 0) {
    LPointer<T, PointerT> p;
    p.ptr_ = AllocatePtr<T, PointerT>(size, p.shm_, alignment);
    return p;
  }

  /**
   * Allocate a pointer of \a size size and return \a p process-independent
   * pointer and its size.
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  Array<PointerT>
  AllocateArray(size_t size, size_t alignment = 0) {
    Array<PointerT> p;
    p.shm_ = Allocate<PointerT>(size, alignment);
    p.size_ = size;
    return p;
  }

  /**
   * Allocate a pointer of \a size size and return \a p process-independent
   * pointer, a process-specific pointer, and its size.
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  LArray<T, PointerT>
  AllocateLocalArray(size_t size, size_t alignment = 0) {
    LArray<T, PointerT> p;
    p.ptr_ = AllocatePtr<T, PointerT>(size, p.shm_, alignment);
    p.size_ = size;
    return p;
  }

  /**
   * Allocate a pointer of \a size size and return \a p process-independent
   * pointer and a process-specific pointer.
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* ClearAllocatePtr(size_t size,
                                         PointerT &p, size_t alignment = 0) {
    p = Allocate<PointerT>(size, alignment);
    if (p.IsNull()) { return nullptr; }
    auto ptr = reinterpret_cast<T*>(buffer_ + p.off_.load());
    if (ptr) {
      memset(ptr, 0, size);
    }
    return ptr;
  }

  /**
   * Allocate a pointer of \a size size
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* ClearAllocatePtr(size_t size, size_t alignment = 0) {
    PointerT p;
    return ClearAllocatePtr<T, PointerT>(size, p, alignment);
  }

  /**
  * Allocate a pointer of \a size size
  * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  LPointer<T, PointerT>
  ClearAllocateLocalPtr(size_t size, size_t alignment = 0) {
    LPointer<T, PointerT> p;
    p.ptr_ = ClearAllocatePtr<T, PointerT>(size, p.shm_, alignment);
    return p;
  }

  /**
   * Allocate a pointer of \a size size
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE Array<PointerT> ClearAllocateArray(
      size_t size, size_t alignment = 0) {
    Array<PointerT> p;
    ClearAllocatePtr<T, PointerT>(size, p.shm_, alignment);
    p.size_ = size;
    return p;
  }

  /**
  * Allocate a pointer of \a size size
  * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  LArray<T, PointerT>
  ClearAllocateLocalArray(size_t size, size_t alignment = 0) {
    LArray<T, PointerT> p;
    p.ptr_ = ClearAllocatePtr<T, PointerT>(size, p.shm_, alignment);
    p.size_ = size;
    return p;
  }

  /**
   * Reallocate a pointer to a new size
   *
   * @param p process-independent pointer (input & output)
   * @param new_size the new size to allocate
   * @param modified whether or not p was modified (output)
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* ReallocatePtr(PointerT &p, size_t new_size,
                                      bool &modified) {
    modified = Reallocate<PointerT>(p, new_size);
    return Convert<T>(p);
  }

  /**
   * Reallocate a pointer to a new size
   *
   * @param p process-independent pointer (input & output)
   * @param new_size the new size to allocate
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* ReallocatePtr(PointerT &p, size_t new_size) {
    Reallocate<PointerT>(p, new_size);
    return Convert<T>(p);
  }

  /**
   * Reallocate a pointer to a new size
   *
   * @param p process-independent pointer (input & output)
   * @param new_size the new size to allocate
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  LPointer<T, PointerT>
  ReallocateLocalPtr(LPointer<T, PointerT> &p, size_t new_size) {
    Reallocate<PointerT>(p.shm_, new_size);
    p.ptr_ = Convert<T>(p.shm_);
    return p;
  }

  /**
   * Reallocate a pointer to a new size
   *
   * @param p process-independent pointer (input & output)
   * @param new_size the new size to allocate
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  Array<PointerT>
  ReallocateArray(Array<PointerT> &p, size_t new_size) {
    Reallocate<PointerT>(p.shm_, new_size);
    p.size_ = new_size;
    return p;
  }

  /**
   * Reallocate a pointer to a new size
   *
   * @param p process-independent pointer (input & output)
   * @param new_size the new size to allocate
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE
  LArray<T, PointerT>
  ReallocateLocalArray(LArray<T, PointerT> &p, size_t new_size) {
    Reallocate<PointerT>(p.shm_, new_size);
    p.ptr_ = Convert<T>(p.shm_);
    p.size_ = new_size;
    return p;
  }

  /**
   * Reallocate a pointer to a new size
   *
   * @param old_ptr process-specific pointer to reallocate
   * @param new_size the new size to allocate
   * @return A process-specific pointer
   * */
  template<typename T>
  HSHM_ALWAYS_INLINE T* ReallocatePtr(T *old_ptr, size_t new_size) {
    OffsetPointer p = Convert<T, OffsetPointer>(old_ptr);
    return ReallocatePtr<T, OffsetPointer>(p, new_size);
  }

  /**
   * Free the memory pointed to by \a ptr Pointer
   * */
  template<typename T = void>
  HSHM_ALWAYS_INLINE void FreePtr(T *ptr) {
    if (ptr == nullptr) {
      throw INVALID_FREE.format();
    }
    FreeOffsetNoNullCheck(Convert<T, OffsetPointer>(ptr));
  }

  /**
   * Free the memory pointed to by \a ptr Pointer
   * */
  template<typename T = void, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE void FreeLocalPtr(LPointer<T, PointerT> &ptr) {
    if (ptr.ptr_ == nullptr) {
      throw INVALID_FREE.format();
    }
    FreeOffsetNoNullCheck(ptr.shm_.ToOffsetPointer());
  }

  /**
   * Free the memory pointed to by \a ptr Pointer
   * */
  template<typename T = void, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE void FreeArray(Array<PointerT> &ptr) {
    if (ptr.shm_.IsNull()) {
      throw INVALID_FREE.format();
    }
    FreeOffsetNoNullCheck(ptr.shm_.ToOffsetPointer());
  }

  /**
   * Free the memory pointed to by \a ptr Pointer
   * */
  template<typename T = void, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE void FreeLocalArray(LArray<T, PointerT> &ptr) {
    if (ptr.ptr_ == nullptr) {
      throw INVALID_FREE.format();
    }
    FreeOffsetNoNullCheck(ptr.shm_.ToOffsetPointer());
  }

  /**====================================
  * Object Allocators
  * ===================================*/

  /**
   * Allocate an array of objects (but don't construct).
   *
   * @return A process-specific pointer
   * */
  template<typename T>
  HSHM_ALWAYS_INLINE T* AllocateObjs(size_t count) {
    OffsetPointer p;
    return AllocateObjs<T>(count, p);
  }

  /**
   * Allocate an array of objects (but don't construct).
   *
   * @return A LocalPointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE LPointer<T, PointerT>
  AllocateObjsLocal(size_t count) {
    LPointer<T, PointerT> p;
    p.ptr_ = AllocateObjs<T>(count, p.shm_);
    return p;
  }

  /**
   * Allocate an array of objects (but don't construct).
   *
   * @param count the number of objects to allocate
   * @param p process-independent pointer (output)
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* AllocateObjs(size_t count, PointerT &p) {
    return AllocatePtr<T>(count * sizeof(T), p);
  }

  /**
   * Allocate an array of objects and memset to 0.
   *
   * @param count the number of objects to allocate
   * @param p process-independent pointer (output)
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* ClearAllocateObjs(size_t count, PointerT &p) {
    return ClearAllocatePtr<T>(count * sizeof(T), p);
  }

  /**
   * Allocate an array of objects and memset to 0.
   *
   * @param count the number of objects to allocate
   * @param p process-independent pointer (output)
   * @return An LPointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE LPointer<T, PointerT>
  ClearAllocateObjsLocal(size_t count) {
    LPointer<T, PointerT> p;
    p.ptr_ = ClearAllocateObjs(count, p.shm_);
    return p;
  }

  /**
   * Allocate and construct an array of objects
   *
   * @param count the number of objects to allocate
   * @param p process-independent pointer (output)
   * @param args parameters to construct object of type T
   * @return A process-specific pointer
   * */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE T* AllocateConstructObjs(size_t count,
                                              PointerT &p, Args&& ...args) {
    T *ptr = AllocateObjs<T>(count, p);
    ConstructObjs<T>(ptr, 0, count, std::forward<Args>(args)...);
    return ptr;
  }

  /**
   * Allocate and construct an array of objects
   *
   * @param count the number of objects to allocate
   * @param p process-independent pointer (output)
   * @param args parameters to construct object of type T
   * @return A process-specific pointer
   * */
  template<
      typename T,
      typename ...Args>
  HSHM_ALWAYS_INLINE T* AllocateConstructObjs(size_t count, Args&& ...args) {
    OffsetPointer p;
    return AllocateConstructObjs<T, OffsetPointer>(count, p,
                                                   std::forward<Args>(args)...);
  }

  /**
   * Allocate and construct an array of objects
   *
   * @param count the number of objects to allocate
   * @param p process-independent pointer (output)
   * @param args parameters to construct object of type T
   * @return A process-specific pointer
   * */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE LPointer<T, PointerT>
  AllocateConstructObjsLocal(size_t count, Args&& ...args) {
    LPointer<T, PointerT> p;
    p.ptr_ = AllocateConstructObjs<T, OffsetPointer>(
        count, p.shm_, std::forward<Args>(args)...);
    return p;
  }

  /** Allocate + construct an array of objects */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE T* NewObjs(size_t count, PointerT &p, Args&& ...args) {
    return AllocateConstructObjs<T>(count, p, std::forward<Args>(args)...);
  }

  /** Allocate + construct an array of objects */
  template<
      typename T,
      typename ...Args>
  HSHM_ALWAYS_INLINE T* NewObjs(size_t count, Args&& ...args) {
    OffsetPointer p;
    return NewObjs<T>(count, p, std::forward<Args>(args)...);
  }

  /** Allocate + construct an array of objects */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE LPointer<T, PointerT>
  NewObjsLocal(size_t count, Args&& ...args) {
    LPointer<T, PointerT> p;
    p.ptr_ = NewObjs<T>(count, p.shm_, std::forward<Args>(args)...);
    return p;
  }

  /** Allocate + construct a single object */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE T* NewObj(PointerT &p, Args&& ...args) {
    return NewObjs<T>(1, p, std::forward<Args>(args)...);
  }

  /** Allocate + construct a single object */
  template<
      typename T,
      typename ...Args>
  HSHM_ALWAYS_INLINE T* NewObj(Args&& ...args) {
    OffsetPointer p;
    return NewObj<T>(p, std::forward<Args>(args)...);
  }

  /** Allocate + construct a single object */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE LPointer<T, PointerT>
  NewObjLocal(Args&& ...args) {
    LPointer<T, PointerT> p;
    p.ptr_ = NewObj<T>(p.shm_, std::forward<Args>(args)...);
    return p;
  }

  /**
   * Reallocate a pointer of objects to a new size.
   *
   * @param p process-independent pointer (input & output)
   * @param old_count the original number of objects (avoids reconstruction)
   * @param new_count the new number of objects
   *
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* ReallocateObjs(PointerT &p, size_t new_count) {
    T *ptr = ReallocatePtr<T>(p, new_count * sizeof(T));
    return ptr;
  }

  /**
   * Reallocate a pointer of objects to a new size.
   *
   * @param p process-independent pointer (input & output)
   * @param old_count the original number of objects (avoids reconstruction)
   * @param new_count the new number of objects
   *
   * @return A process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE void
  ReallocateObjsLocal(LPointer<T, PointerT> &p, size_t new_count) {
    p.ptr_ = ReallocatePtr<T>(p.shm_, new_count * sizeof(T));
  }

  /**
   * Reallocate a pointer of objects to a new size and construct the
   * new elements in-place.
   *
   * @param p process-independent pointer (input & output)
   * @param old_count the original number of objects (avoids reconstruction)
   * @param new_count the new number of objects
   * @param args parameters to construct object of type T
   *
   * @return A process-specific pointer
   * */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE T* ReallocateConstructObjs(PointerT &p,
                                                size_t old_count,
                                                size_t new_count,
                                                Args&& ...args) {
    T *ptr = ReallocatePtr<T>(p, new_count * sizeof(T));
    ConstructObjs<T>(ptr, old_count, new_count, std::forward<Args>(args)...);
    return ptr;
  }

  /**
  * Reallocate a pointer of objects to a new size and construct the
  * new elements in-place.
  *
  * @param p process-independent pointer (input & output)
  * @param old_count the original number of objects (avoids reconstruction)
  * @param new_count the new number of objects
  * @param args parameters to construct object of type T
  *
  * @return A process-specific pointer
  * */
  template<
      typename T,
      typename PointerT = Pointer,
      typename ...Args>
  HSHM_ALWAYS_INLINE void
  ReallocateConstructObjsLocal(LPointer<T, PointerT> &p,
                               size_t old_count,
                               size_t new_count,
                               Args&& ...args) {
    p.ptr_ = ReallocateConstructObjs<T>(p.shm_, old_count, new_count,
                                        std::forward<Args>(args)...);
  }

  /**====================================
  * Object Deallocators
  * ===================================*/

  /**
   * Free + destruct objects
   * */
  template <typename T>
  HSHM_ALWAYS_INLINE void FreeDestructObjs(T *ptr, size_t count) {
    DestructObjs<T>(ptr, count);
    auto p = Convert<T, OffsetPointer>(ptr);
    Free(p);
  }

  /**
   * Free + destruct objects
   * */
  template <typename T, typename PointerT>
  HSHM_ALWAYS_INLINE void
  FreeDestructObjsLocal(LPointer<T, PointerT> &p, size_t count) {
    DestructObjs<T>(p.ptr_, count);
    Free(p.shm_);
  }

  /**
   * Free + destruct objects
   * */
  template <typename T>
  HSHM_ALWAYS_INLINE void DelObjs(T *ptr, size_t count) {
    FreeDestructObjs<T>(ptr, count);
  }

  /**
   * Free + destruct objects
   * */
  template <typename T, typename PointerT>
  HSHM_ALWAYS_INLINE void
  DelObjsLocal(LPointer<T, PointerT> &p, size_t count) {
    FreeDestructObjsLocal<T>(p, count);
  }

  /**
   * Free + destruct an object
   * */
  template <typename T>
  HSHM_ALWAYS_INLINE void DelObj(T *ptr) {
    FreeDestructObjs<T>(ptr, 1);
  }

  /**
   * Free + destruct an object
   * */
  template <typename T, typename PointerT>
  HSHM_ALWAYS_INLINE void DelObjLocal(LPointer<T, PointerT> &p) {
    FreeDestructObjsLocal<T>(p, 1);
  }


  /**====================================
  * Object Constructors
  * ===================================*/

  /**
   * Construct each object in an array of objects.
   *
   * @param ptr the array of objects (potentially archived)
   * @param old_count the original size of the ptr
   * @param new_count the new size of the ptr
   * @param args parameters to construct object of type T
   * @return None
   * */
  template<
      typename T,
      typename ...Args>
  HSHM_ALWAYS_INLINE static void
  ConstructObjs(T *ptr,
                size_t old_count,
                size_t new_count, Args&& ...args) {
    if (ptr == nullptr) { return; }
    for (size_t i = old_count; i < new_count; ++i) {
      ConstructObj<T>(*(ptr + i), std::forward<Args>(args)...);
    }
  }

  /**
   * Construct an object.
   *
   * @param ptr the object to construct (potentially archived)
   * @param args parameters to construct object of type T
   * @return None
   * */
  template<
      typename T,
      typename ...Args>
  HSHM_ALWAYS_INLINE static void
  ConstructObj(T &obj, Args&& ...args) {
    new (&obj) T(std::forward<Args>(args)...);
  }

  /**
   * Destruct an array of objects
   *
   * @param ptr the object to destruct (potentially archived)
   * @param count the length of the object array
   * @return None
   * */
  template<typename T>
  HSHM_ALWAYS_INLINE static void
  DestructObjs(T *ptr, size_t count) {
    if (ptr == nullptr) { return; }
    for (size_t i = 0; i < count; ++i) {
      DestructObj<T>(*(ptr + i));
    }
  }

  /**
   * Destruct an object
   *
   * @param ptr the object to destruct (potentially archived)
   * @param count the length of the object array
   * @return None
   * */
  template<typename T>
  HSHM_ALWAYS_INLINE static void DestructObj(T &obj) {
    obj.~T();
  }

  /**====================================
  * Helpers
  * ===================================*/

  /**
   * Get the custom header of the shared-memory allocator
   *
   * @return Custom header pointer
   * */
  template<typename HEADER_T>
  HSHM_ALWAYS_INLINE HEADER_T* GetCustomHeader() {
    return reinterpret_cast<HEADER_T*>(custom_header_);
  }

  /**
   * Convert a process-independent pointer into a process-specific pointer
   *
   * @param p process-independent pointer
   * @return a process-specific pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE T* Convert(const PointerT &p) {
    if (p.IsNull()) { return nullptr; }
    return reinterpret_cast<T*>(buffer_ + p.off_.load());
  }

  /**
   * Convert a process-specific pointer into a process-independent pointer
   *
   * @param ptr process-specific pointer
   * @return a process-independent pointer
   * */
  template<typename T, typename PointerT = Pointer>
  HSHM_ALWAYS_INLINE PointerT Convert(const T *ptr) {
    if (ptr == nullptr) { return PointerT::GetNull(); }
    return PointerT(GetId(),
                     reinterpret_cast<size_t>(ptr) -
                         reinterpret_cast<size_t>(buffer_));
  }

  /**
   * Determine whether or not this allocator contains a process-specific
   * pointer
   *
   * @param ptr process-specific pointer
   * @return True or false
   * */
  template<typename T = void>
  HSHM_ALWAYS_INLINE bool ContainsPtr(T *ptr) {
    return  reinterpret_cast<size_t>(ptr) >=
        reinterpret_cast<size_t>(buffer_);
  }
};

}  // namespace hshm::ipc

#endif  // HERMES_MEMORY_ALLOCATOR_ALLOCATOR_H_
