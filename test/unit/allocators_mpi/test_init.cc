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


#include "basic_test.h"
#include "test_init.h"

Allocator *alloc_g = nullptr;

void PretestRankN() {
  std::string shm_url = "test_allocators";
  allocator_id_t alloc_id(0, 1);
  auto mem_mngr = HERMES_MEMORY_MANAGER;
  mem_mngr->UnregisterAllocator(alloc_id);
  mem_mngr->UnregisterBackend(shm_url);
  mem_mngr->AttachBackend(MemoryBackendType::kPosixShmMmap, shm_url);
  alloc_g = mem_mngr->GetAllocator(alloc_id);
}

void MainPretest() {
}

void MainPosttest() {
}
