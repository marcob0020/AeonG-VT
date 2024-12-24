//
// Created by marcob0020 on 12/22/24.
//

#include "comparers.hpp"

#include "edge.hpp"
#include "vertex.hpp"

int storage::EdgeStoreTypeComparer::operator()(const EdgeStoreType &lhs, const EdgeStoreType &rhs) const {
  //std::tuple<EdgeTypeId, Vertex *, EdgeRef>
  const auto lhs_t = std::get<0>(lhs);
  const auto rhs_t = std::get<0>(rhs);

  if (lhs_t != rhs_t)
    return lhs_t.AsInt() - rhs_t.AsInt();

  const auto lhs_v = std::get<1>(lhs);
  const auto rhs_v = std::get<1>(rhs);

  if (lhs_v != rhs_v)
    return lhs_v->gid.AsInt() - rhs_v->gid.AsInt();

  const auto lhs_e = std::get<2>(lhs);
  const auto rhs_e = std::get<2>(rhs);

  if (lhs_e != rhs_e)
    return lhs_e.gid.AsInt() - rhs_e.gid.AsInt();

  return 0;
}

int storage::SetComparer::operator()(const vt_checks_set_t &a, const vt_checks_set_t &b) const{
  Delta::Action a_0 = std::get<0>(a);
  Delta::Action b_0 = std::get<0>(b);

  if (a_0 != b_0)
    return static_cast<int>(a_0) - static_cast<int>(b_0);

  const std::variant<Vertex*, Edge*> a_1 = std::get<1>(a);
  const std::variant<Vertex*, Edge*> b_1 = std::get<1>(b);

  if (a_1.index() != b_1.index())
    return a_1.index() - b_1.index();

  if (a_1.index() == 0) {
    const auto a_1_v = std::get<Vertex*>(a_1);
    const auto b_1_v = std::get<Vertex*>(b_1);

    if (a_1_v != b_1_v)
      return a_1_v->gid.AsInt() - b_1_v->gid.AsInt();
  }else if (a_1.index() == 1) {
    const auto a_1_e = std::get<Edge*>(a_1);
    const auto b_1_e = std::get<Edge*>(b_1);

    if (a_1_e != b_1_e)
      return a_1_e->gid.AsInt() - b_1_e->gid.AsInt();
  }



  const std::variant<LabelId,PropertyId,EdgeStoreType, std::monostate> a_2 = std::get<2>(a);
  const std::variant<LabelId,PropertyId,EdgeStoreType, std::monostate> b_2 = std::get<2>(b);

  if (a_2.index() != b_2.index())
    return a_2.index() - b_2.index();

  if (a_2.index() == 0) {
    return std::get<LabelId>(a_2).AsInt() - std::get<LabelId>(b_2).AsInt();
  }

  if (a_2.index() == 1) {
    return std::get<PropertyId>(a_2).AsInt() - std::get<PropertyId>(b_2).AsInt();
  }

  if (a_2.index() == 2) {
    const std::tuple<EdgeTypeId, Vertex *, EdgeRef> a_2_e = std::get<EdgeStoreType> (a_2);
    const std::tuple<EdgeTypeId, Vertex *, EdgeRef> b_2_e = std::get<EdgeStoreType> (b_2);

    return EdgeStoreTypeComparer()(a_2_e, b_2_e);
  }

  return 0;
}


