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


#ifndef HERMES_MEMORY_MEMORY_H_
#define HERMES_MEMORY_MEMORY_H_

#include <hermes_shm/types/real_number.h>
#include <hermes_shm/constants/data_structure_singleton_macros.h>
#include <hermes_shm/introspect/system_info.h>
#include <hermes_shm/types/bitfield.h>
#include <hermes_shm/types/atomic.h>
#include <hermes_shm/constants/macros.h>

namespace hshm::ipc {

/**
 * The identifier for an allocator
 * */
union allocator_id_t {
  struct {
    uint32_t major_;  // Typically some sort of process id
    uint32_t minor_;  // Typically a process-local id
  } bits_;
  uint64_t int_;

  HSHM_ALWAYS_INLINE allocator_id_t() = default;

  /**
   * Constructor which sets major & minor
   * */
  HSHM_ALWAYS_INLINE explicit allocator_id_t(uint32_t major, uint32_t minor) {
    bits_.major_ = major;
    bits_.minor_ = minor;
  }

  /**
   * Set this allocator to null
   * */
  HSHM_ALWAYS_INLINE void SetNull() {
    int_ = 0;
  }

  /**
   * Check if this is the null allocator
   * */
  HSHM_ALWAYS_INLINE bool IsNull() const { return int_ == 0; }

  /** Equality check */
  HSHM_ALWAYS_INLINE bool operator==(const allocator_id_t &other) const {
    return other.int_ == int_;
  }

  /** Inequality check */
  HSHM_ALWAYS_INLINE bool operator!=(const allocator_id_t &other) const {
    return other.int_ != int_;
  }

  /** Get the null allocator */
  HSHM_ALWAYS_INLINE static allocator_id_t GetNull() {
    static allocator_id_t alloc(0, 0);
    return alloc;
  }

  /** To index */
  HSHM_ALWAYS_INLINE uint32_t ToIndex() {
    return bits_.major_ * 4 + bits_.minor_;
  }

  /** Serialize an hipc::allocator_id */
  template <typename Ar>
  HSHM_ALWAYS_INLINE
  void serialize(Ar &ar) {
    ar &int_;
  }

  /** String printing */
  friend std::ostream& operator<<(std::ostream &os, const allocator_id_t &id) {
    return os << (std::to_string(id.bits_.major_) + "."
        + std::to_string(id.bits_.minor_));
  }
};

typedef uint32_t slot_id_t;  // Uniquely ids a MemoryBackend slot

/**
 * Stores an offset into a memory region. Assumes the developer knows
 * which allocator the pointer comes from.
 * */
template<bool ATOMIC = false>
struct OffsetPointerBase {
  typedef typename std::conditional<ATOMIC,
    atomic<size_t>, nonatomic<size_t>>::type atomic_t;
  atomic_t off_; /**< Offset within the allocator's slot */

  /** Default constructor */
  HSHM_ALWAYS_INLINE OffsetPointerBase() = default;

  /** Full constructor */
  HSHM_ALWAYS_INLINE explicit OffsetPointerBase(size_t off) : off_(off) {}

  /** Full constructor */
  HSHM_ALWAYS_INLINE explicit OffsetPointerBase(atomic_t off)
  : off_(off.load()) {}

  /** Pointer constructor */
  HSHM_ALWAYS_INLINE explicit OffsetPointerBase(allocator_id_t alloc_id,
                                                size_t off)
  : off_(off) {
    (void) alloc_id;
  }

  /** Copy constructor */
  HSHM_ALWAYS_INLINE OffsetPointerBase(const OffsetPointerBase &other)
  : off_(other.off_.load()) {}

  /** Other copy constructor */
  HSHM_ALWAYS_INLINE OffsetPointerBase(const OffsetPointerBase<!ATOMIC> &other)
  : off_(other.off_.load()) {}

  /** Move constructor */
  HSHM_ALWAYS_INLINE OffsetPointerBase(OffsetPointerBase &&other) noexcept
    : off_(other.off_.load()) {
    other.SetNull();
  }

  /** Get the offset pointer */
  HSHM_ALWAYS_INLINE OffsetPointerBase<false> ToOffsetPointer() {
    return OffsetPointerBase<false>(off_.load());
  }

  /** Set to null */
  HSHM_ALWAYS_INLINE void SetNull() {
    off_ = (size_t)-1;
  }

  /** Check if null */
  HSHM_ALWAYS_INLINE bool IsNull() const {
    return off_.load() == (size_t)-1;
  }

  /** Get the null pointer */
  HSHM_ALWAYS_INLINE static OffsetPointerBase GetNull() {
    static const OffsetPointerBase p(-1);
    return p;
  }

  /** Atomic load wrapper */
  HSHM_ALWAYS_INLINE size_t load(
    std::memory_order order = std::memory_order_seq_cst) const {
    return off_.load(order);
  }

  /** Atomic exchange wrapper */
  HSHM_ALWAYS_INLINE void exchange(
    size_t count, std::memory_order order = std::memory_order_seq_cst) {
    off_.exchange(count, order);
  }

  /** Atomic compare exchange weak wrapper */
  HSHM_ALWAYS_INLINE bool compare_exchange_weak(
    size_t& expected, size_t desired,
    std::memory_order order = std::memory_order_seq_cst) {
    return off_.compare_exchange_weak(expected, desired, order);
  }

  /** Atomic compare exchange strong wrapper */
  HSHM_ALWAYS_INLINE bool compare_exchange_strong(
    size_t& expected, size_t desired,
    std::memory_order order = std::memory_order_seq_cst) {
    return off_.compare_exchange_weak(expected, desired, order);
  }

  /** Atomic add operator */
  HSHM_ALWAYS_INLINE OffsetPointerBase operator+(size_t count) const {
    return OffsetPointerBase(off_ + count);
  }

  /** Atomic subtract operator */
  HSHM_ALWAYS_INLINE OffsetPointerBase operator-(size_t count) const {
    return OffsetPointerBase(off_ - count);
  }

  /** Atomic add assign operator */
  HSHM_ALWAYS_INLINE OffsetPointerBase& operator+=(size_t count) {
    off_ += count;
    return *this;
  }

  /** Atomic subtract assign operator */
  HSHM_ALWAYS_INLINE OffsetPointerBase& operator-=(size_t count) {
    off_ -= count;
    return *this;
  }

  /** Atomic assign operator */
  HSHM_ALWAYS_INLINE OffsetPointerBase& operator=(size_t count) {
    off_ = count;
    return *this;
  }

  /** Atomic copy assign operator */
  HSHM_ALWAYS_INLINE OffsetPointerBase& operator=(
    const OffsetPointerBase &count) {
    off_ = count.load();
    return *this;
  }

  /** Equality check */
  HSHM_ALWAYS_INLINE bool operator==(const OffsetPointerBase &other) const {
    return off_ == other.off_;
  }

  /** Inequality check */
  HSHM_ALWAYS_INLINE bool operator!=(const OffsetPointerBase &other) const {
    return off_ != other.off_;
  }
};

/** Non-atomic offset */
typedef OffsetPointerBase<false> OffsetPointer;

/** Atomic offset */
typedef OffsetPointerBase<true> AtomicOffsetPointer;

/** Typed offset pointer */
template<typename T>
using TypedOffsetPointer = OffsetPointer;

/** Typed atomic pointer */
template<typename T>
using TypedAtomicOffsetPointer = AtomicOffsetPointer;

/**
 * A process-independent pointer, which stores both the allocator's
 * information and the offset within the allocator's region
 * */
template<bool ATOMIC = false>
struct PointerBase {
  allocator_id_t allocator_id_;     /// Allocator the pointer comes from
  OffsetPointerBase<ATOMIC> off_;   /// Offset within the allocator's slot

  /** Default constructor */
  HSHM_ALWAYS_INLINE PointerBase() = default;

  /** Full constructor */
  HSHM_ALWAYS_INLINE explicit PointerBase(allocator_id_t id, size_t off)
  : allocator_id_(id), off_(off) {}

  /** Full constructor using offset pointer */
  HSHM_ALWAYS_INLINE explicit PointerBase(allocator_id_t id, OffsetPointer off)
  : allocator_id_(id), off_(off) {}

  /** Copy constructor */
  HSHM_ALWAYS_INLINE PointerBase(const PointerBase &other)
  : allocator_id_(other.allocator_id_), off_(other.off_) {}

  /** Other copy constructor */
  HSHM_ALWAYS_INLINE PointerBase(const PointerBase<!ATOMIC> &other)
  : allocator_id_(other.allocator_id_), off_(other.off_.load()) {}

  /** Move constructor */
  HSHM_ALWAYS_INLINE PointerBase(PointerBase &&other) noexcept
  : allocator_id_(other.allocator_id_), off_(other.off_) {
    other.SetNull();
  }

  /** Get the offset pointer */
  HSHM_ALWAYS_INLINE OffsetPointerBase<false> ToOffsetPointer() const {
    return OffsetPointerBase<false>(off_.load());
  }

  /** Set to null */
  HSHM_ALWAYS_INLINE void SetNull() {
    allocator_id_.SetNull();
  }

  /** Check if null */
  HSHM_ALWAYS_INLINE bool IsNull() const {
    return allocator_id_.IsNull();
  }

  /** Get the null pointer */
  HSHM_ALWAYS_INLINE static PointerBase GetNull() {
    static const PointerBase p(allocator_id_t::GetNull(),
                               OffsetPointer::GetNull());
    return p;
  }

  /** Copy assignment operator */
  HSHM_ALWAYS_INLINE PointerBase& operator=(const PointerBase &other) {
    if (this != &other) {
      allocator_id_ = other.allocator_id_;
      off_ = other.off_;
    }
    return *this;
  }

  /** Move assignment operator */
  HSHM_ALWAYS_INLINE PointerBase& operator=(PointerBase &&other) {
    if (this != &other) {
      allocator_id_ = other.allocator_id_;
      off_.exchange(other.off_.load());
      other.SetNull();
    }
    return *this;
  }

  /** Addition operator */
  HSHM_ALWAYS_INLINE PointerBase operator+(size_t size) const {
    PointerBase p;
    p.allocator_id_ = allocator_id_;
    p.off_ = off_ + size;
    return p;
  }

  /** Subtraction operator */
  HSHM_ALWAYS_INLINE PointerBase operator-(size_t size) const {
    PointerBase p;
    p.allocator_id_ = allocator_id_;
    p.off_ = off_ - size;
    return p;
  }

  /** Addition assignment operator */
  HSHM_ALWAYS_INLINE PointerBase& operator+=(size_t size) {
    off_ += size;
    return *this;
  }

  /** Subtraction assignment operator */
  HSHM_ALWAYS_INLINE PointerBase& operator-=(size_t size) {
    off_ -= size;
    return *this;
  }

  /** Equality check */
  HSHM_ALWAYS_INLINE bool operator==(const PointerBase &other) const {
    return (other.allocator_id_ == allocator_id_ && other.off_ == off_);
  }

  /** Inequality check */
  HSHM_ALWAYS_INLINE bool operator!=(const PointerBase &other) const {
    return (other.allocator_id_ != allocator_id_ || other.off_ != off_);
  }

  /** String printing */
  friend std::ostream& operator<<(std::ostream &os, const PointerBase &p) {
    return os << p.allocator_id_ << "."
        + std::to_string(p.off_.load());
  }
};

/** Non-atomic pointer */
typedef PointerBase<false> Pointer;

/** Atomic pointer */
typedef PointerBase<true> AtomicPointer;

/** Typed pointer */
template<typename T>
using TypedPointer = Pointer;

/** Typed atomic pointer */
template<typename T>
using TypedAtomicPointer = AtomicPointer;

/** Struct containing both private and shared pointer */
template<typename T = char, typename PointerT = Pointer>
struct LPointer {
  T *ptr_;
  PointerT shm_;

  /** Overload arrow */
  HSHM_ALWAYS_INLINE T* operator->() const {
    return ptr_;
  }

  /** Overload dereference */
  HSHM_ALWAYS_INLINE T& operator*() const {
    return *ptr_;
  }

  /** Check if null */
  HSHM_ALWAYS_INLINE bool IsNull() const {
    return ptr_ == nullptr;
  }

  /** Set to null */
  HSHM_ALWAYS_INLINE void SetNull() {
    ptr_ = nullptr;
  }

  /** Implicit copy type cast */
  template<typename ConvT>
  HSHM_ALWAYS_INLINE operator LPointer<ConvT, PointerT>() const {
    return LPointer<ConvT, PointerT>{(ConvT*)ptr_, shm_};
  }
};

/** Struct containing both a pointer and its size */
template<typename PointerT = Pointer>
struct Array {
  PointerT shm_;
  size_t size_;
};

/** Struct containing a shared pointer, private pointer, and the data size */
template<typename T = char, typename PointerT = Pointer>
struct LArray {
  PointerT shm_;
  size_t size_;
  T *ptr_;

  /** Overload arrow */
  HSHM_ALWAYS_INLINE T* operator->() const {
    return ptr_;
  }

  /** Overload dereference */
  HSHM_ALWAYS_INLINE T& operator*() const {
    return *ptr_;
  }
};

class MemoryAlignment {
 public:
  /**
   * Round up to the nearest multiple of the alignment
   * @param alignment the alignment value (e.g., 4096)
   * @param size the size to make a multiple of alignment (e.g., 4097)
   * @return the new size  (e.g., 8192)
   * */
  HSHM_ALWAYS_INLINE static size_t AlignTo(size_t alignment,
                                           size_t size) {
    auto page_size = HERMES_SYSTEM_INFO->page_size_;
    size_t new_size = size;
    size_t page_off = size % alignment;
    if (page_off) {
      new_size = size + page_size - page_off;
    }
    return new_size;
  }

  /**
   * Round up to the nearest multiple of page size
   * @param size the size to align to the PAGE_SIZE
   * */
  HSHM_ALWAYS_INLINE static size_t AlignToPageSize(size_t size) {
    auto page_size = HERMES_SYSTEM_INFO->page_size_;
    size_t new_size = AlignTo(page_size, size);
    return new_size;
  }
};

}  // namespace hshm::ipc

namespace std {

/** Allocator ID hash */
template <>
struct hash<hshm::ipc::allocator_id_t> {
  HSHM_ALWAYS_INLINE std::size_t operator()(
    const hshm::ipc::allocator_id_t &key) const {
    return std::hash<uint64_t>{}(key.int_);
  }
};

}  // namespace std

#define IS_SHM_OFFSET_POINTER(T) \
  std::is_same_v<T, OffsetPointer> || \
  std::is_same_v<T, AtomicOffsetPointer>

#define IS_SHM_POINTER(T) \
  std::is_same_v<T, Pointer> || \
  std::is_same_v<T, AtomicPointer>

#endif  // HERMES_MEMORY_MEMORY_H_
