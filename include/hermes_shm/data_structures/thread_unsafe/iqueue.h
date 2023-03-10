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


#ifndef HERMES_DATA_STRUCTURES_THREAD_UNSAFE_IQUEUE_H
#define HERMES_DATA_STRUCTURES_THREAD_UNSAFE_IQUEUE_H

#include "hermes_shm/data_structures/internal/shm_internal.h"

namespace hermes_shm::ipc {

/** forward pointer for iqueue */
template<typename T>
class iqueue;

/** represents an object within a iqueue */
struct iqueue_entry {
  OffsetPointer next_ptr_;
};

/**
 * The iqueue iterator
 * */
template<typename T>
struct iqueue_iterator_templ {
 public:
  /**< A shm reference to the containing iqueue object. */
  hipc::ShmRef<iqueue<T>> iqueue_;
  /**< A pointer to the entry in shared memory */
  iqueue_entry *entry_;
  /**< A pointer to the entry prior to this one */
  iqueue_entry *prior_entry_;

  /** Default constructor */
  iqueue_iterator_templ() = default;

  /** Construct end iterator */
  explicit iqueue_iterator_templ(bool)
  : entry_(nullptr) {}

  /** Construct begin iterator  */
  explicit iqueue_iterator_templ(ShmDeserialize<iqueue<T>> iqueue,
                                 iqueue_entry *entry)
    : iqueue_(iqueue), entry_(entry), prior_entry_(nullptr) {}

  /** Copy constructor */
  iqueue_iterator_templ(const iqueue_iterator_templ &other) {
    iqueue_ = other.iqueue_;
    entry_ = other.entry_;
    prior_entry_ = other.prior_entry_;
  }

  /** Assign this iterator from another iterator */
  iqueue_iterator_templ& operator=(const iqueue_iterator_templ &other) {
    if (this != &other) {
      iqueue_ = other.iqueue_;
      entry_ = other.entry_;
      prior_entry_ = other.prior_entry_;
    }
    return *this;
  }

  /** Get the object the iterator points to */
  T* operator*() {
    return reinterpret_cast<T*>(entry_);
  }

  /** Get the object the iterator points to */
  const T* operator*() const {
    return reinterpret_cast<T*>(entry_);
  }

  /** Get the next iterator (in place) */
  iqueue_iterator_templ& operator++() {
    if (is_end()) { return *this; }
    prior_entry_ = entry_;
    entry_ = iqueue_->alloc_->template
      Convert<iqueue_entry>(entry_->next_ptr_);
    return *this;
  }

  /** Return the next iterator */
  iqueue_iterator_templ operator++(int) const {
    iqueue_iterator_templ next_iter(*this);
    ++next_iter;
    return next_iter;
  }

  /** Return the iterator at count after this one */
  iqueue_iterator_templ operator+(size_t count) const {
    iqueue_iterator_templ pos(*this);
    for (size_t i = 0; i < count; ++i) {
      ++pos;
    }
    return pos;
  }

  /** Get the iterator at count after this one (in-place) */
  void operator+=(size_t count) {
    iqueue_iterator_templ pos = (*this) + count;
    entry_ = pos.entry_;
    prior_entry_ = pos.prior_entry_;
  }

  /** Determine if two iterators are equal */
  friend bool operator==(const iqueue_iterator_templ &a,
                         const iqueue_iterator_templ &b) {
    return (a.is_end() && b.is_end()) || (a.entry_ == b.entry_);
  }

  /** Determine if two iterators are inequal */
  friend bool operator!=(const iqueue_iterator_templ &a,
                         const iqueue_iterator_templ &b) {
    return !(a.is_end() && b.is_end()) && (a.entry_ != b.entry_);
  }

  /** Create the end iterator */
  static iqueue_iterator_templ const end() {
    static const iqueue_iterator_templ end_iter(true);
    return end_iter;
  }

  /** Determine whether this iterator is the end iterator */
  bool is_end() const {
    return entry_ == nullptr;
  }

  /** Determine whether this iterator is the begin iterator */
  bool is_begin() const {
    if (entry_) {
      return prior_entry_ == nullptr;
    } else {
      return false;
    }
  }
};

/** forward iterator typedef */
template<typename T>
using iqueue_iterator = iqueue_iterator_templ<T>;

/** const forward iterator typedef */
template<typename T>
using iqueue_citerator = iqueue_iterator_templ<T>;


/**
 * MACROS used to simplify the iqueue namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME iqueue
#define TYPED_CLASS iqueue<T>
#define TYPED_HEADER ShmHeader<iqueue<T>>

/**
 * The iqueue shared-memory header
 * */
template<typename T>
struct ShmHeader<iqueue<T>> : public ShmBaseHeader {
  OffsetPointer head_ptr_;
  size_t length_;

  /** Default constructor */
  ShmHeader() = default;

  /** Copy constructor */
  ShmHeader(const ShmHeader &other) {
    strong_copy(other);
  }

  /** Copy assignment operator */
  ShmHeader& operator=(const ShmHeader &other) {
    if (this != &other) {
      strong_copy(other);
    }
    return *this;
  }

  /** Strong copy operation */
  void strong_copy(const ShmHeader &other) {
    head_ptr_ = other.head_ptr_;
    length_ = other.length_;
  }

  /** Move constructor */
  ShmHeader(ShmHeader &&other) {
    weak_move(other);
  }

  /** Move operator */
  ShmHeader& operator=(ShmHeader &&other) {
    if (this != &other) {
      weak_move(other);
    }
    return *this;
  }

  /** Move operation */
  void weak_move(ShmHeader &other) {
    strong_copy(other);
  }
};

/**
 * Doubly linked iqueue implementation
 * */
template<typename T>
class iqueue : public ShmContainer {
 public:
  SHM_CONTAINER_TEMPLATE((CLASS_NAME), (TYPED_CLASS), (TYPED_HEADER))

 public:
  ////////////////////////////
  /// SHM Overrides
  ////////////////////////////

  /** Default constructor */
  iqueue() = default;

  /** Initialize iqueue in shared memory */
  void shm_init_main(TYPED_HEADER *header,
                     Allocator *alloc) {
    shm_init_allocator(alloc);
    shm_init_header(header);
    header_->length_ = 0;
    header_->head_ptr_.SetNull();
  }

  /** Destroy all shared memory allocated by the iqueue */
  void shm_destroy_main() {
    clear();
  }

  /** Store into shared memory */
  void shm_serialize_main() const {}

  /** Load from shared memory */
  void shm_deserialize_main() {}

  /** Move constructor */
  void shm_weak_move_main(TYPED_HEADER *header,
                          Allocator *alloc, iqueue &&other) {}

  /** Copy constructor */
  void shm_strong_copy_main(TYPED_HEADER *header,
                            Allocator *alloc, const iqueue &other) {}

  ////////////////////////////
  /// iqueue Methods
  ////////////////////////////

  /** Construct an element at \a pos position in the iqueue */
  void enqueue(T *entry) {
    OffsetPointer entry_ptr = alloc_->
      template Convert<T, OffsetPointer>(entry);
    reinterpret_cast<iqueue_entry*>(entry)->next_ptr_ = header_->head_ptr_;
    header_->head_ptr_ = entry_ptr;
    ++header_->length_;
  }

  /** Dequeue the first element */
  T* dequeue() {
    if (size() == 0) { return nullptr; }
    auto entry = alloc_->
      template Convert<iqueue_entry>(header_->head_ptr_);
    header_->head_ptr_ = entry->next_ptr_;
    --header_->length_;
    return reinterpret_cast<T*>(entry);
  }

  /** Dequeue the element at the iterator position */
  T* dequeue(iqueue_iterator_templ<T> pos) {
    if (pos.prior_entry_ == nullptr) {
      return dequeue();
    }
    auto entry = *pos;
    auto prior_cast = reinterpret_cast<iqueue_entry*>(pos.prior_entry_);
    auto pos_cast = reinterpret_cast<iqueue_entry*>(pos.entry_);
    prior_cast->next_ptr_ = pos_cast->next_ptr_;
    --header_->length_;
    return reinterpret_cast<T*>(entry);
  }

  /** Peek the first element of the queue */
  T* peek() {
    if (size() == 0) { return nullptr; }
    auto entry = alloc_->
      template Convert<iqueue_entry>(header_->head_ptr_);
    return reinterpret_cast<T*>(entry);
  }

  /** Destroy all elements in the iqueue */
  void clear() {
    while (size()) {
      dequeue();
    }
  }

  /** Get the number of elements in the iqueue */
  size_t size() const {
    if (!IsNull()) {
      return header_->length_;
    }
    return 0;
  }

  /**
   * ITERATORS
   * */

  /** Forward iterator begin */
  iqueue_iterator<T> begin() {
    if (size() == 0) { return end(); }
    auto head = alloc_->template
      Convert<iqueue_entry>(header_->head_ptr_);
    return iqueue_iterator<T>(GetShmDeserialize(), head);
  }

  /** Forward iterator end */
  static iqueue_iterator<T> const end() {
    return iqueue_iterator<T>::end();
  }

  /** Constant forward iterator begin */
  iqueue_citerator<T> cbegin() const {
    if (size() == 0) { return cend(); }
    auto head = alloc_->template
      Convert<iqueue_entry>(header_->head_ptr_);
    return iqueue_citerator<T>(GetShmDeserialize(), head);
  }

  /** Constant forward iterator end */
  static iqueue_citerator<T> const cend() {
    return iqueue_citerator<T>::end();
  }
};

}  // namespace hermes_shm::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // HERMES_DATA_STRUCTURES_THREAD_UNSAFE_IQUEUE_H
