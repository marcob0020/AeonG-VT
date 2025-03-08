// Copyright 2021 Memgraph Ltd.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.txt; by using this file, you agree to be bound by the terms of the Business Source
// License, and you may not use this file except in compliance with the Business Source License.
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0, included in the file
// licenses/APL.txt.

#pragma once

#include <limits>
#include <tuple>
#include <vector>

#include "storage/v2/delta.hpp"
#include "storage/v2/edge_ref.hpp"
#include "storage/v2/id_types.hpp"
#include "storage/v2/property_store.hpp"
#include "storage/v2/vt_store.hpp"
#include "utils/spin_lock.hpp"

namespace storage {

class VtStore;

struct Vertex {
  Vertex(const Vertex&) = delete;
  Vertex(Vertex&& other) {
    gid = other.gid;
    delta = other.delta;
    transaction_st=0;
    ve_tt_ts=0;
    num=0;
  }

  Vertex(Gid gid, Delta *delta) : gid(gid), deleted(false), delta(delta), vt_store(nullptr) {
    transaction_st=0;
    ve_tt_ts=0;
    num=0;
    MG_ASSERT(delta == nullptr || delta->action == Delta::Action::DELETE_OBJECT,
              "Vertex must be created with an initial DELETE_OBJECT delta!");
  }
  Vertex(Gid gid, Delta *delta,uint64_t transaction_st) : gid(gid),deleted(false),transaction_st(transaction_st),delta(delta), vt_store(nullptr){
    transaction_st=0;
    num=0;
    ve_tt_ts=0;
  }
  ~Vertex() {
    if (vt_store != nullptr) {
      delete vt_store;
      vt_store = nullptr;
    }
  }

  storage::VtStore& get_vt_store() {
    if (vt_store == nullptr) {
      vt_store = new VtStore();
    }
    return *vt_store;
  }

  Gid gid;

  std::vector<LabelId> labels;
  PropertyStore properties;

  std::vector<std::tuple<EdgeTypeId, Vertex *, EdgeRef>> in_edges;
  std::vector<std::tuple<EdgeTypeId, Vertex *, EdgeRef>> out_edges;

  mutable utils::SpinLock lock;
  bool deleted;

  int has_vt = 0;

  int num;
  uint64_t transaction_st;
  uint64_t ve_tt_ts;
  Delta *delta;

  storage::VtStore* vt_store;
};

static_assert(alignof(Vertex) >= 8, "The Vertex should be aligned to at least 8!");

inline bool operator==(const Vertex &first, const Vertex &second) { return first.gid == second.gid; }
inline bool operator<(const Vertex &first, const Vertex &second) { return first.gid < second.gid; }
inline bool operator==(const Vertex &first, const Gid &second) { return first.gid == second; }
inline bool operator<(const Vertex &first, const Gid &second) { return first.gid < second; }

}  // namespace storage
