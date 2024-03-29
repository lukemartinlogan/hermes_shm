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

#ifndef HERMES_SHM_INCLUDE_HERMES_SHM_DATA_STRUCTURES_IPC_mpsc_ptr_queue_H_
#define HERMES_SHM_INCLUDE_HERMES_SHM_DATA_STRUCTURES_IPC_mpsc_ptr_queue_H_

#include "hermes_shm/data_structures/ipc/internal/shm_internal.h"
#include "hermes_shm/thread/lock.h"
#include "vector.h"
#include "pair.h"
#include "hermes_shm/types/qtok.h"

namespace hshm::ipc {

/** Forward declaration of mpsc_ptr_queue */
template<typename T>
class mpsc_ptr_queue;

/**
 * MACROS used to simplify the mpsc_ptr_queue namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME mpsc_ptr_queue
#define TYPED_CLASS mpsc_ptr_queue<T>
#define TYPED_HEADER ShmHeader<mpsc_ptr_queue<T>>

/**
 * A queue optimized for multiple producers (emplace) with a single
 * consumer (pop).
 * */
template<typename T>
class mpsc_ptr_queue : public ShmContainer {
 public:
  SHM_CONTAINER_TEMPLATE((CLASS_NAME), (TYPED_CLASS))
  ShmArchive<vector<T>> queue_;
  std::atomic<_qtok_t> tail_;
  std::atomic<_qtok_t> head_;
  RwLock lock_;
  bitfield32_t flags_;

 public:
  /**====================================
   * Default Constructor
   * ===================================*/

  /** SHM constructor. Default. */
  explicit mpsc_ptr_queue(Allocator *alloc,
                          size_t depth = 1024) {
    shm_init_container(alloc);
    HSHM_MAKE_AR(queue_, GetAllocator(), depth);
    flags_.Clear();
    SetNull();
  }

  /**====================================
   * Copy Constructors
   * ===================================*/

  /** SHM copy constructor */
  explicit mpsc_ptr_queue(Allocator *alloc,
                          const mpsc_ptr_queue &other) {
    shm_init_container(alloc);
    SetNull();
    shm_strong_copy_construct_and_op(other);
  }

  /** SHM copy assignment operator */
  mpsc_ptr_queue& operator=(const mpsc_ptr_queue &other) {
    if (this != &other) {
      shm_destroy();
      shm_strong_copy_construct_and_op(other);
    }
    return *this;
  }

  /** SHM copy constructor + operator main */
  void shm_strong_copy_construct_and_op(const mpsc_ptr_queue &other) {
    head_ = other.head_.load();
    tail_ = other.tail_.load();
    (*queue_) = (*other.queue_);
  }

  /**====================================
   * Move Constructors
   * ===================================*/

  /** SHM move constructor. */
  mpsc_ptr_queue(Allocator *alloc,
                 mpsc_ptr_queue &&other) noexcept {
    shm_init_container(alloc);
    if (GetAllocator() == other.GetAllocator()) {
      head_ = other.head_.load();
      tail_ = other.tail_.load();
      (*queue_) = std::move(*other.queue_);
      other.SetNull();
    } else {
      shm_strong_copy_construct_and_op(other);
      other.shm_destroy();
    }
  }

  /** SHM move assignment operator. */
  mpsc_ptr_queue& operator=(mpsc_ptr_queue &&other) noexcept {
    if (this != &other) {
      shm_destroy();
      if (GetAllocator() == other.GetAllocator()) {
        head_ = other.head_.load();
        tail_ = other.tail_.load();
        (*queue_) = std::move(*other.queue_);
        other.SetNull();
      } else {
        shm_strong_copy_construct_and_op(other);
        other.shm_destroy();
      }
    }
    return *this;
  }

  /**====================================
   * Destructor
   * ===================================*/

  /** SHM destructor.  */
  void shm_destroy_main() {
    (*queue_).shm_destroy();
  }

  /** Check if the list is empty */
  bool IsNull() const {
    return (*queue_).IsNull();
  }

  /** Sets this list as empty */
  void SetNull() {
    head_ = 0;
    tail_ = 0;
  }

  /**====================================
   * MPSC Queue Methods
   * ===================================*/

  /** Construct an element at \a pos position in the list */
  template<typename ...Args>
  qtok_t emplace(const T &val) {
    // Allocate a slot in the queue
    // The slot is marked NULL, so pop won't do anything if context switch
    _qtok_t head = head_.load();
    _qtok_t tail = tail_.fetch_add(1);
    size_t size = tail - head + 1;
    vector<T> &queue = (*queue_);

    // Check if there's space in the queue.
    if (size > queue.size()) {
      while (true) {
        head = head_.load();
        size = tail - head + 1;
        if (size <= (*queue_).size()) {
          break;
        }
        HERMES_THREAD_MODEL->Yield();
      }
    }

    // Emplace into queue at our slot
    uint32_t idx = tail % queue.size();
    if constexpr(std::is_arithmetic<T>::value) {
      queue[idx] = MARK_FIRST_BIT(T, val);
    } else if constexpr(IS_SHM_OFFSET_POINTER(T)) {
      queue[idx] = T(MARK_FIRST_BIT(size_t, val.off_.load()));
    } else if constexpr(IS_SHM_POINTER(T)) {
      queue[idx] = T(val.allocator_id_,
                     MARK_FIRST_BIT(size_t, val.off_.load()));
    }

    // Let pop know that the data is fully prepared
    return qtok_t(tail);
  }

 public:
  /** Consumer pops the head object */
  qtok_t pop(T &val) {
    // Don't pop if there's no entries
    _qtok_t head = head_.load();
    _qtok_t tail = tail_.load();
    if (head >= tail) {
      return qtok_t::GetNull();
    }

    // Pop the element, but only if it's marked valid
    _qtok_t idx = head % (*queue_).size();
    T &entry = (*queue_)[idx];

    // Check if bit is marked
    bool is_marked;
    if constexpr(std::is_arithmetic<T>::value) {
      is_marked = IS_FIRST_BIT_MARKED(T, entry);
    } else {
      is_marked = IS_FIRST_BIT_MARKED(size_t, entry.off_.load());
    }

    // Complete dequeue if marked
    if (is_marked) {
      if constexpr(std::is_arithmetic<T>::value) {
        val = UNMARK_FIRST_BIT(T, entry);
        entry = 0;
      } else if constexpr(IS_SHM_OFFSET_POINTER(T)) {
        val = T(UNMARK_FIRST_BIT(size_t, entry.off_.load()));
        entry.off_ = 0;
      } else if constexpr(IS_SHM_POINTER(T)) {
        val = T(entry.allocator_id_,
                UNMARK_FIRST_BIT(size_t, entry.off_.load()));
        entry.off_ = 0;
      }
      head_.fetch_add(1);
      return qtok_t(head);
    } else {
      return qtok_t::GetNull();
    }
  }
};

}  // namespace hshm::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // HERMES_SHM_INCLUDE_HERMES_SHM_DATA_STRUCTURES_IPC_mpsc_ptr_queue_H_
