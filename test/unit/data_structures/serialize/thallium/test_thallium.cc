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
#include "hermes_shm/data_structures/string.h"

TEST_CASE("SerializeString") {
  tl::endpoint server = client_->lookup(kServerName);
  tl::remote_procedure string0_proc = client_->define(kStringTest0);
  tl::remote_procedure string_large_proc = client_->define(kStringTestLarge);

  hipc::string empty_str("");
  hipc::string large_str(kTestString);

  REQUIRE(string0_proc.on(server)(empty_str));
  REQUIRE(string_large_proc.on(server)(large_str));
}

TEST_CASE("SerializeCharBuf") {
  tl::endpoint server = client_->lookup(kServerName);
  tl::remote_procedure string0_proc = client_->define(kCharbufTest0);
  tl::remote_procedure string_large_proc = client_->define(kCharbufTestLarge);

  hipc::string empty_str("");
  hipc::string large_str(kTestString);

  REQUIRE(string0_proc.on(server)(empty_str));
  REQUIRE(string_large_proc.on(server)(large_str));
}

TEST_CASE("SerializeVectorOfInt") {
  tl::endpoint server = client_->lookup(kServerName);
  tl::remote_procedure vec_int0_proc = client_->define(kVecOfInt0Test);
  tl::remote_procedure vec_int_proc = client_->define(kVecOfIntLargeTest);

  // Send empty vector
  hipc::vector<int> vec_int(0);
  REQUIRE(vec_int0_proc.on(server)(vec_int));

  // Send initialized vector
  for (int i = 0; i < 20; ++i) {
    vec_int.emplace_back(i);
  }
  REQUIRE(vec_int_proc.on(server)(vec_int));
}

TEST_CASE("SerializeVectorOfString") {
  tl::endpoint server = client_->lookup(kServerName);
  tl::remote_procedure vec_string0_proc = client_->define(kVecOfString0Test);
  tl::remote_procedure vec_string_proc = client_->define(kVecOfStringLargeTest);

  // Send empty vector
  hipc::vector<hipc::string> vec_string(0);
  REQUIRE(vec_string0_proc.on(server)(vec_string));

  // Send initialized vector
  for (int i = 0; i < 20; ++i) {
    vec_string.emplace_back(std::to_string(i));
  }
  REQUIRE(vec_string_proc.on(server)(vec_string));
}

TEST_CASE("SerializeBitfield") {
  tl::endpoint server = client_->lookup(kServerName);
  tl::remote_procedure bitfield_proc = client_->define(kBitfieldTest);

  // Send bitfield
  hshm::bitfield32_t field;
  field.SetBits(0x8);
  REQUIRE(bitfield_proc.on(server)(field));
}

TEST_CASE("SerializeVectorOfIntRef") {
  tl::endpoint server = client_->lookup(kServerName);
  tl::remote_procedure vec_int_proc = client_->define(kVecOfIntRefTest);

  // Send empty vector
  hipc::vector<int> vec_int(0);
  for (int i = 0; i < 20; ++i) {
    vec_int.emplace_back(i);
  }

  hipc::ShmRef<hipc::vector<int>> vec_ref(vec_int.GetShmDeserialize());
  REQUIRE(vec_int_proc.on(server)(vec_ref));
}
