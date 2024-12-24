//
// Created by marcob0020 on 12/22/24.
//

#ifndef COMPARERS_HPP
#define COMPARERS_HPP
#include <tuple>
#include <variant>

#include "delta.hpp"

namespace storage {
  struct EdgeRef;
  struct Vertex;
  class EdgeTypeId;

  using EdgeStoreType = std::tuple<storage::EdgeTypeId, storage::Vertex *, storage::EdgeRef>;

  using obj_t = std::variant<Vertex*, Edge*>;
  using add_info_t = std::variant<LabelId,PropertyId,EdgeStoreType, std::monostate>;
  using vt_checks_set_t = std::tuple<Delta::Action, obj_t, add_info_t>;


struct EdgeStoreTypeComparer {
  int operator()(const EdgeStoreType& lhs, const EdgeStoreType& rhs) const;
};

struct SetComparer {
  int operator()(const vt_checks_set_t& lhs, const vt_checks_set_t& rhs) const;
};


}
#endif //COMPARERS_HPP
