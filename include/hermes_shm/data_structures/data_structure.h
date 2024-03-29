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


#ifndef HERMES_DATA_STRUCTURES_DATA_STRUCTURE_H_
#define HERMES_DATA_STRUCTURES_DATA_STRUCTURE_H_

#include "ipc/internal/shm_internal.h"
#include "hermes_shm/memory/memory_manager.h"

#include "containers/charbuf.h"
#include "containers/converters.h"
#include "containers/functional.h"
#include "containers/tuple_base.h"

#include "ipc/pair.h"
#include "ipc/string.h"
#include "ipc/list.h"
#include "ipc/vector.h"
#include "ipc/mpsc_queue.h"
#include "ipc/slist.h"
#include "ipc/split_ticket_queue.h"
#include "ipc/spsc_queue.h"
#include "ipc/ticket_queue.h"
#include "ipc/unordered_map.h"
#include "ipc/pod_array.h"

#include "serialization/serialize_common.h"

namespace hipc = hshm::ipc;

#endif  // HERMES_DATA_STRUCTURES_DATA_STRUCTURE_H_
