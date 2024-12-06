//
// Created by marcob0020 on 11/3/24.
//

#include "vt_store.hpp"

#include <utils/temporal_filter.hpp>
#include <utils/temporal_functions.hpp>

namespace storage {

  VtStore::VtStore() {
    DeleteObject(utils::TimeSpan());
  }

  VtStore::VtStore(VtStore &&other) noexcept {
    lifetime_ = std::move(other.lifetime_);
    ingoing_edges_ = std::move(other.ingoing_edges_);
    outgoing_edges_ = std::move(other.outgoing_edges_);
    properties_ = std::move(other.properties_);
  }

  VtStore &VtStore::operator=(VtStore &&other) noexcept {
    lifetime_ = std::move(other.lifetime_);
    ingoing_edges_ = std::move(other.ingoing_edges_);
    outgoing_edges_ = std::move(other.outgoing_edges_);
    properties_ = std::move(other.properties_);
    return *this;
  }

  utils::valued_timeline<PropertyValue> VtStore::GetProperty(PropertyId property, const utils::TimeSpan &vt) const {
    utils::valued_timeline<PropertyValue> result(vt);

    auto it = properties_.find(property);

    if (it == properties_.end())
      return result;

    return utils::ValuedTimelineRetrieval<ValuedTimeline,PropertyValue>(vt, it->second);
  }

  bool VtStore::HasProperty(PropertyId property, const utils::TimeSpan& vt) const {
    auto it = properties_.find(property);

    if (it == properties_.end())
      return false;

    return utils::ValuedTimelineExistence(vt, it->second);
  }

  bool VtStore::HasProperty(PropertyId property) const {
    auto it = properties_.find(property);

    return it != properties_.end() && !it->second.empty();
  }

  bool VtStore::IsPropertyEqual(PropertyId property, const PropertyValue &value, const utils::TimeSpan &vt) const {
    auto it = properties_.find(property);

    if (it == properties_.end())
      return value.IsNull();

    if (value.IsNull())
      return true;

    return utils::ValuedTimelineEquals(vt, it->second, value);
  }

  std::vector<PropertyId> VtStore::Properties() const {
    std::vector<PropertyId> result;

    for (const auto &kv : properties_) {
      result.push_back(kv.first);
    }

    return result;
  }

  bool VtStore::SetProperty(PropertyId property, const PropertyValue &value, const utils::TimeSpan &vt) {
    auto it = properties_.find(property);

    if (it == properties_.end()) {
      std::vector<std::pair<utils::TimeSpan,PropertyValue>> v;
      v.emplace_back(vt, value);
      properties_.emplace(property, std::move(v));

      return true;
    }

    return utils::ValuedTimelineInsertion(vt, false, it->second, value);
  }

  bool VtStore::ClearProperties() {
    bool empty = properties_.empty();

    properties_.clear();

    return !empty;
  }

  bool VtStore::ClearProperties(utils::TimeSpan &vt) {

    bool edit = false;
    for (auto it = properties_.begin(); it != properties_.end(); it++) {
      if (it->second.empty())
        properties_.erase(it);

      else
        edit |= SetProperty(it->first,PropertyValue(),vt);
    }

    return edit;
  }

  utils::timeline VtStore::GetOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan &vt) const {
    const auto it = outgoing_edges_.find(edge);

    if (it == outgoing_edges_.end())
      return {};


    return utils::TimelineRetrieval(vt, it->second);
  }

  utils::timeline VtStore::GetIngoingEdge(EdgeStoreType edge, const utils::TimeSpan &vt) const {
    const auto it = ingoing_edges_.find(edge);

    if (it == outgoing_edges_.end())
      return {};

    return utils::TimelineRetrieval(vt, it->second);
  }

  bool VtStore::HasOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt) const {
    const auto it = outgoing_edges_.find(edge);

    if (it == outgoing_edges_.end())
      return false;

    return utils::TimelineExistence(vt, it->second);
  }

  bool VtStore::HasIngoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt) const {
    const auto it = ingoing_edges_.find(edge);

    if (it == ingoing_edges_.end())
      return false;

    return utils::TimelineExistence(vt, it->second);
  }

  bool VtStore::HasOutgoingEdge(EdgeStoreType edge) const {
    const auto it = outgoing_edges_.find(edge);

    return it != outgoing_edges_.end();
  }

  bool VtStore::HasIngoingEdge(EdgeStoreType edge) const {
    const auto it = ingoing_edges_.find(edge);

    return it != ingoing_edges_.end();
  }

  std::vector<VtStore::EdgeStoreType> VtStore::OutgoingEdges() const {
    std::vector<EdgeStoreType> result;

    for (const auto &kv : outgoing_edges_) {
      result.push_back(kv.first);
    }

    return result;
  }

  std::vector<VtStore::EdgeStoreType> VtStore::IngoingEdges() const {
    std::vector<EdgeStoreType> result;

    for (const auto &kv : ingoing_edges_) {
      result.push_back(kv.first);
    }

    return result;
  }

  bool VtStore::SetOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan &vt) {
    const auto it = outgoing_edges_.find(edge);

    if (it==outgoing_edges_.end()) {
      std::vector<utils::TimeSpan> new_edges;
      new_edges.emplace_back(vt);
      outgoing_edges_[edge] = new_edges;

      return true;
    }

    return utils::TimelineInsertion(vt, false, it->second);
  }

  bool VtStore::DeleteOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan &vt) {
    const auto it = outgoing_edges_.find(edge);

    if (it==outgoing_edges_.end())
      return true;

    return utils::TimelineInsertion(vt, true, it->second);

  }


  bool VtStore::SetIngoingEdge(EdgeStoreType edge, const utils::TimeSpan &vt) {
    const auto it = ingoing_edges_.find(edge);

    if (it==ingoing_edges_.end()) {
      std::vector<utils::TimeSpan> new_edges;
      new_edges.emplace_back(vt);
      ingoing_edges_[edge] = new_edges;

      return true;
    }

    return utils::TimelineInsertion(vt, false, it->second);
  }

  bool VtStore::DeleteIngoingEdge(EdgeStoreType edge, const utils::TimeSpan &vt) {
    const auto it = ingoing_edges_.find(edge);

    if (it==ingoing_edges_.end())
      return true;

    return utils::TimelineInsertion(vt, true, it->second);
  }


 utils::timeline VtStore::GetObjectValidity(const utils::TimeSpan &vt) const {
    return utils::TimelineRetrieval(vt, lifetime_);
 }

 bool VtStore::CreateObject(const utils::TimeSpan &vt) {
   return utils::TimelineInsertion(vt, false, lifetime_);
 }

  bool VtStore::DeleteObject(const utils::TimeSpan &vt) {
    return utils::TimelineInsertion(vt, true, lifetime_);
  }

  bool VtStore::DeleteObject() {
    return utils::TimelineInsertion(utils::TimeSpan() , true, lifetime_);
  }

  utils::timeline VtStore::GetLabel(LabelId label, const utils::TimeSpan &vt) const {
    const auto it = labels_.find(label);

    if (it == labels_.end())
      return {};

    return utils::TimelineRetrieval(vt, it->second);
  }

  bool VtStore::HasLabel(LabelId label, const utils::TimeSpan &vt) const {
    const auto it = labels_.find(label);

    if (it == labels_.end())
      return false;

    return utils::TimelineExistence(vt, it->second);
  }

  bool VtStore::HasLabel(LabelId label) const {
    const auto it = labels_.find(label);

    return it != labels_.end();
  }

  std::vector<LabelId> VtStore::Labels() const {
    std::vector<LabelId> result;

    for (const auto &kv : labels_) {
      result.push_back(kv.first);
    }

    return result;
  }

  bool VtStore::SetLabel(LabelId label, const utils::TimeSpan &vt) {
    const auto it = labels_.find(label);

    if (it==labels_.end()) {
      std::vector<utils::TimeSpan> new_Labels;
      new_Labels.emplace_back(vt);
      labels_[label] = new_Labels;

      return true;
    }

    return utils::TimelineInsertion(vt, false, it->second);
  }

  bool VtStore::DeleteLabel(LabelId label, const utils::TimeSpan &vt) {
    const auto it = labels_.find(label);

    if (it==labels_.end())
      return true;

    return utils::TimelineInsertion(vt, true, it->second);
  }

  bool VtStore::DeleteLabel(LabelId label) {
    return DeleteLabel(label, utils::TimeSpan());
  }



} // storage