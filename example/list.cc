//
// Created by lukemartinlogan on 2/22/23.
//

#include <mpi.h>
#include <cassert>
#include "hermes_shm/data_structures/thread_unsafe/list.h"
#include "hermes_shm/data_structures/data_structure.h"

struct CustomHeader {
  hipc::TypedPointer<hipc::list<int>> obj_;
};

int main(int argc, char **argv) {
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Common allocator information
  std::string shm_url = "test_allocators";
  hipc::allocator_id_t alloc_id(0, 1);
  auto mem_mngr = HERMES_MEMORY_MANAGER;
  hipc::Allocator *alloc;
  CustomHeader *header;

  // Create backend + allocator
  if (rank == 0) {
    // Create a 64 megabyte allocatable region
    mem_mngr->CreateBackend<hipc::PosixShmMmap>(
      MEGABYTES(64), shm_url);
    // Create a memory allocator over the 64MB region
    alloc = mem_mngr->CreateAllocator<hipc::StackAllocator>(
      shm_url, alloc_id, sizeof(CustomHeader));
    // Get the custom header from the allocator
    header = alloc->GetCustomHeader<CustomHeader>();
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Attach backend + find allocator
  if (rank != 0) {
    mem_mngr->AttachBackend(hipc::MemoryBackendType::kPosixShmMmap, shm_url);
    alloc = mem_mngr->GetAllocator(alloc_id);
    header = alloc->GetCustomHeader<CustomHeader>();
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Create the list
  hipc::list<int> obj;
  if (rank == 0) {
    // Initialize in shared memory
    obj.shm_init(alloc);
    // Save the list inside the allocator's header
    header->obj_ = obj.GetShmPointer<hipc::Pointer>();
    // Emplace 1024 elements
    for (int i = 0; i < 1024; ++i) {
      obj.emplace_back(10);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Find the list in shared memory
  if (rank != 0) {
    obj << header->obj_;
  }

  // Read list on all ranks
  for (hipc::ShmRef<int> x : obj) {
    assert(*x == 10);
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Finalize
  if (rank == 0) {
    std::cout << "COMPLETE!" << std::endl;
  }
  MPI_Finalize();
}