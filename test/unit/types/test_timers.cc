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

#include <unistd.h>
#include <hermes_shm/util/timer.h>
#include <hermes_shm/util/timer_mpi.h>
#include <hermes_shm/util/timer_thread.h>
#include <hermes_shm/util/logging.h>
#include "basic_test.h"

TEST_CASE("TestTimepoint") {
  hshm::Timepoint timer;
  timer.Now();
  sleep(2);
  HILOG(kInfo, "Print timer: {}", timer.GetSecFromStart());
}

TEST_CASE("TestTimer") {
  hshm::Timer timer;
  timer.Resume();
  sleep(3);
  timer.Pause();
  HILOG(kInfo, "Print timer: {}", timer.GetSec());
}

TEST_CASE("TestMpiTimer") {
  hshm::MpiTimer mpi_timer(MPI_COMM_WORLD);
  mpi_timer.Resume();
  sleep(3);
  mpi_timer.Pause();
  mpi_timer.Collect();
  HILOG(kInfo, "Print timer: {}", mpi_timer.GetSec());
}

TEST_CASE("TestOmpTimer") {
  hshm::ThreadTimer omp_timer(4);
#pragma omp parallel shared(omp_timer) num_threads(4)
  {
    omp_timer.SetRank(omp_get_thread_num());
    omp_timer.Resume();
    sleep(3);
    omp_timer.Pause();
#pragma omp barrier
  }
  omp_timer.Collect();
  HILOG(kInfo, "Print timer: {}", omp_timer.GetSec());
}
