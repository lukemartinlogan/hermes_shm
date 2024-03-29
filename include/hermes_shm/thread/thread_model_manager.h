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


#ifndef HERMES_THREAD_THREAD_MANAGER_H_
#define HERMES_THREAD_THREAD_MANAGER_H_

#include "hermes_shm/thread/thread_model/thread_model.h"
#include "hermes_shm/thread/thread_model/thread_model_factory.h"
#include <hermes_shm/introspect/system_info.h>
#include <mutex>

#include "hermes_shm/util/singleton/_global_singleton.h"
#define HERMES_THREAD_MODEL \
  hshm::GlobalSingleton<hshm::ThreadModelManager>::GetInstance()
#define HERMES_THREAD_MODEL_T \
  hshm::ThreadModelManager*

namespace hshm {

class ThreadModelManager {
 public:
  ThreadType type_; /**< The type of threads used in this program */
  std::unique_ptr<thread_model::ThreadModel>
    thread_static_; /**< Functions static to all threads */

  /** Default constructor */
  ThreadModelManager() {
    SetThreadModel(ThreadType::kPthread);
  }

  /** Set the threading model of this application */
  void SetThreadModel(ThreadType type);

  /** Sleep for a period of time (microseconds) */
  void SleepForUs(size_t us);

  /** Call Yield */
  void Yield();

  /** Call GetTid */
  tid_t GetTid();
};

/** A unique identifier of this thread across processes */
union NodeThreadId {
  struct {
    uint32_t tid_;
    uint32_t pid_;
  } bits_;
  uint64_t as_int_;

  /** Default constructor */
  NodeThreadId() {
    bits_.tid_ = HERMES_THREAD_MODEL->GetTid();
    bits_.pid_ = HERMES_SYSTEM_INFO->pid_;
  }

  /** Hash function */
  uint32_t hash() {
    return as_int_;
  }
};

}  // namespace hshm

#endif  // HERMES_THREAD_THREAD_MANAGER_H_
