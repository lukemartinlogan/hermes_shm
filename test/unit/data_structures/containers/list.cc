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
#include "list.h"
#include "hermes_shm/data_structures/ipc/list.h"
#include "hermes_shm/data_structures/ipc/string.h"

using hshm::ipc::list;

template<typename T>
void ListTestRunner(ListTestSuite<T, list<T>> &test) {
  test.EmplaceTest(15);
  test.ForwardIteratorTest();
  test.ConstForwardIteratorTest();
  test.CopyConstructorTest();
  test.CopyAssignmentTest();
  test.MoveConstructorTest();
  test.MoveAssignmentTest();
  test.EmplaceFrontTest();
  test.ModifyEntryCopyIntoTest();
  test.ModifyEntryMoveIntoTest();
  test.EraseTest();
}

template<typename T>
void ListTest() {
  Allocator *alloc = alloc_g;
  auto lp = hipc::make_uptr<list<T>>(alloc);
  ListTestSuite<T, list<T>> test(*lp, alloc);
  ListTestRunner(test);
}

TEST_CASE("ListOfInt") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ListTest<int>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("ListOfString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ListTest<hipc::string>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}

TEST_CASE("ListOfStdString") {
  Allocator *alloc = alloc_g;
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
  ListTest<std::string>();
  REQUIRE(alloc->GetCurrentlyAllocatedSize() == 0);
}
