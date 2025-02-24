//
// Created by marcob0020 on 11/3/24.
//

#include "vt_store.hpp"

#include <query/serialization/property_value.hpp>
#include <utils/temporal_functions.hpp>

#include "vertex.hpp"
#include "durability/exceptions.hpp"
#include "durability/marker.hpp"
#include "durability/serialization.hpp"

namespace storage {



  VtStore::VtStore(){
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

  bool VtStore::InitProperty(PropertyId property, const PropertyValue &value) {
    if (!properties_.contains(property)) {
      std::vector<std::pair<utils::TimeSpan,PropertyValue>> v;
      v.emplace_back(utils::TimeSpan(), value);
      properties_.emplace(property, std::move(v));

      return true;
    }
    return false;
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

  bool VtStore::IsValid() const {
    return !lifetime_.empty();
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

  namespace serialization {
    void SerializeList(durability::BaseEncoder *encoder, const VtStore::TimelineList &list) {
      encoder->WriteUint(list.size());
      for (const auto &vt : list) {
        encoder->WriteUint(vt.first.get_microseconds());
        encoder->WriteUint(vt.second.get_microseconds());
      }
    }

    static const int kInitialAllocationFactorTimeSpan = sizeof(uint64_t) * 2 + 2;
    static const int kInitialAllocationFactorPropertyValue = sizeof(PropertyValue) + 1;

    std::string SerializeList(const VtStore::TimelineList &list) {
      std::string result;
      result.reserve(list.size() * kInitialAllocationFactorTimeSpan);

      for (const auto &vt : list) {
        result+= std::to_string(utils::MemcpyCast<uint64_t>(vt.first.get_microseconds())) + ":";
        result+= std::to_string(utils::MemcpyCast<uint64_t>(vt.second.get_microseconds())) + ";";
      }

      return result;
    }



    std::string SerializeValuedList(const VtStore::ValuedTimeline &list) {
      std::string result;
      result.reserve(list.size() * kInitialAllocationFactorPropertyValue);

      for (const auto &vt : list) {
        result+= std::to_string(utils::MemcpyCast<uint64_t>(vt.first.first.get_microseconds())) + ":";
        result+= std::to_string(utils::MemcpyCast<uint64_t>(vt.first.second.get_microseconds())) + ":";
        result+= query::serialization::SerializePropertyValue(vt.second).dump();
      }

      return result;
    }

    void SerializeValuedList(durability::BaseEncoder *encoder, const VtStore::ValuedTimeline &list) {
      encoder->WriteUint(list.size());
      for (const auto &vt : list) {
        encoder->WriteUint(vt.first.first.get_microseconds());
        encoder->WriteUint(vt.first.second.get_microseconds());
        encoder->WritePropertyValue(vt.second);
      }
    }

    void SerializeObjectValidity(durability::BaseEncoder *encoder, const VtStore::TimelineList &validity) {
      if (validity.empty())
        return;
      encoder->WriteMarker(durability::Marker::VTSTORE_OBJECT_VALIDITY);
      SerializeList(encoder, validity);
    }

    void SerializeEdges(durability::BaseEncoder *encoder, const std::map<EdgeStoreType, VtStore::TimelineList, EdgeStoreTypeComparer> &edges, durability::Marker edge_type) {
      if (edges.empty())
        return;

      for (auto it = edges.begin(); it!=edges.end(); it++) {
        const EdgeStoreType edge = it->first;
        const VtStore::TimelineList &list = it->second;

        if (list.empty())
          continue;

        const EdgeTypeId edge_type_id = std::get<0>(edge);
        const Vertex* vertex = std::get<1>(edge);
        const EdgeRef& ref = std::get<2>(edge);

        encoder->WriteMarker(edge_type);
        encoder->WriteUint(edge_type_id.AsUint());
        encoder->WriteUint(vertex->gid.AsUint());
        encoder->WriteUint(ref.gid.AsUint());
        SerializeList(encoder, list);
      }
    }

    void SerializeEdges(const std::map<EdgeStoreType, VtStore::TimelineList, EdgeStoreTypeComparer> &edges, const std::string& prefix, std::map<std::string, std::string> &result) {
      for (auto it = edges.begin(); it!=edges.end(); it++) {
        const EdgeStoreType edge = it->first;
        const VtStore::TimelineList &list = it->second;

        if (list.empty())
          continue;

        const EdgeTypeId edge_type_id = std::get<0>(edge);
        const Vertex* vertex = std::get<1>(edge);
        const EdgeRef& ref = std::get<2>(edge);

        std::string prefix2 = prefix + std::to_string(edge_type_id.AsUint()) + ":" + std::to_string(vertex->gid.AsUint()) + ":" + std::to_string(ref.gid.AsUint());
        result.emplace(prefix2, SerializeList(list));
      }
    }



    void SerializeProperties(durability::BaseEncoder *encoder, const std::map<PropertyId, VtStore::ValuedTimeline> &props) {
      if (props.empty())
        return;

      for (auto it = props.begin(); it!=props.end(); it++) {
        const PropertyId property_id = it->first;
        const VtStore::ValuedTimeline &valued_list = it->second;

        if (valued_list.empty())
          continue;

        encoder->WriteMarker(durability::Marker::VTSTORE_PROPERTY);
        encoder->WriteUint(property_id.AsUint());
        SerializeValuedList(encoder,valued_list);
      }
    }

    void SerializeProperties(const std::map<PropertyId, VtStore::ValuedTimeline> &props, const std::string &prefix, std::map<std::string, std::string> &result) {

      for (auto it = props.begin(); it!=props.end(); it++) {
        const PropertyId property_id = it->first;
        const VtStore::ValuedTimeline &valued_list = it->second;

        if (valued_list.empty())
          continue;

        std::string prefix2 = prefix + std::to_string(property_id.AsUint());
        result.emplace(prefix2, SerializeValuedList(valued_list));
      }
    }

    void DeserializeList(durability::BaseDecoder *decoder, VtStore::TimelineList &list) {
      std::optional<uint64_t> size = decoder->ReadUint();
      if (!size) throw durability::RecoveryFailure("Invalid data in VT recovery!");

      list.clear();
      list.reserve(size.value());

      for (int i = 0;i!=size;i++) {
        const std::optional<uint64_t> first = decoder->ReadUint();
        if (!first) throw durability::RecoveryFailure("Invalid data in VT recovery!");
        const std::optional<uint64_t> second = decoder->ReadUint();
        if (!second) throw durability::RecoveryFailure("Invalid data in VT recovery!");

        list.emplace_back(utils::VTDateTime(utils::MemcpyCast<int64_t>(*first)),utils::VTDateTime(utils::MemcpyCast<int64_t>(*second)));
      }
    }

    void DeserializeListNull(durability::BaseDecoder *decoder) {
      std::optional<uint64_t> size = decoder->ReadUint();
      if (!size) throw durability::RecoveryFailure("Invalid data in VT recovery!");

      for (int i = 0;i!=size;i++) {
        const std::optional<uint64_t> first = decoder->ReadUint();
        if (!first) throw durability::RecoveryFailure("Invalid data in VT recovery!");
        const std::optional<uint64_t> second = decoder->ReadUint();
        if (!second) throw durability::RecoveryFailure("Invalid data in VT recovery!");

      }
    }

    void DeserializeValuedList(durability::BaseDecoder *decoder, VtStore::ValuedTimeline &list) {
      std::optional<uint64_t> size = decoder->ReadUint();
      if (!size) throw durability::RecoveryFailure("Invalid data in VT recovery!");

      list.clear();
      list.reserve(size.value());

      for (int i = 0;i!=size;i++) {
        const std::optional<uint64_t> first = decoder->ReadUint();
        if (!first) throw durability::RecoveryFailure("Invalid data in VT recovery!");
        const std::optional<uint64_t> second = decoder->ReadUint();
        if (!second) throw durability::RecoveryFailure("Invalid data in VT recovery!");
        const std::optional<PropertyValue> prop = decoder->ReadPropertyValue();
        if (!prop) throw durability::RecoveryFailure("Invalid data in VT recovery!");


        list.emplace_back(std::make_pair(utils::TimeSpan(utils::VTDateTime(utils::MemcpyCast<int64_t>(*first)),utils::VTDateTime(utils::MemcpyCast<int64_t>(*second))),*prop));
      }
    }

    void DeserializeValuedListNull(durability::BaseDecoder *decoder) {
      std::optional<uint64_t> size = decoder->ReadUint();
      if (!size) throw durability::RecoveryFailure("Invalid data in VT recovery!");

      for (int i = 0;i!=size;i++) {
        const std::optional<uint64_t> first = decoder->ReadUint();
        if (!first) throw durability::RecoveryFailure("Invalid data in VT recovery!");
        const std::optional<uint64_t> second = decoder->ReadUint();
        if (!second) throw durability::RecoveryFailure("Invalid data in VT recovery!");
        bool prop = decoder->SkipPropertyValue();
        if (!prop) throw durability::RecoveryFailure("Invalid data in VT recovery!");

       }
    }
  }

  void VtStore::SerializeToWriter(durability::BaseEncoder *encoder) const {
    encoder->WriteMarker(durability::Marker::SECTION_VTSTORE);
    serialization::SerializeObjectValidity(encoder, lifetime_);
    serialization::SerializeEdges(encoder, ingoing_edges_, durability::Marker::VTSTORE_IN_EDGE);
    serialization::SerializeEdges(encoder, outgoing_edges_, durability::Marker::VTSTORE_OUT_EDGE);
    serialization::SerializeProperties(encoder, properties_);
    encoder->WriteMarker(durability::Marker::VTSTORE_END);
  }

  std::map<std::string,std::string> VtStore::SerializeToStrings() const {
    std::map<std::string, std::string> result;

    if (!lifetime_.empty())
      result.emplace("OBJ", serialization::SerializeList(lifetime_));

    if (!ingoing_edges_.empty())
      serialization::SerializeEdges(ingoing_edges_, "IE", result);

    if (!outgoing_edges_.empty())
      serialization::SerializeEdges(outgoing_edges_, "OE", result);

    if (!properties_.empty())
      serialization::SerializeProperties(properties_, "P", result);

    return result;
  }

  void VtStore::DeserializeIntoValidity(durability::BaseDecoder *decoder) {
    serialization::DeserializeList(decoder, lifetime_);
  }

  void VtStore::DeserializeIntoInEdges(durability::BaseDecoder *decoder, std::optional<EdgeStoreType> edge) {
    if (edge == std::nullopt)
      serialization::DeserializeListNull(decoder);
    else
      serialization::DeserializeList(decoder, ingoing_edges_[*edge]);
  }

  void VtStore::DeserializeIntoOutEdges(durability::BaseDecoder *decoder, std::optional<EdgeStoreType> edge) {
    if (edge == std::nullopt)
      serialization::DeserializeListNull(decoder);
    else
      serialization::DeserializeList(decoder, outgoing_edges_[*edge]);
  }

  void VtStore::DeserializeIntoProperty(durability::BaseDecoder *decoder, std::optional<PropertyId> property) {
    if (property == std::nullopt)
      serialization::DeserializeValuedListNull(decoder);
    else
      serialization::DeserializeValuedList(decoder, properties_[*property]);
  }

  // void VtStore::DeserializeIntoValidity(std::string val) {
  //
  // }
  // void VtStore::DeserializeIntoInEdges(std::string val, std::optional<EdgeStoreType> edge);
  // void VtStore::DeserializeIntoOutEdges(std::string val, std::optional<EdgeStoreType> edge);
  // void VtStore::DeserializeIntoProperty(std::string val, std::optional<PropertyId> property);



  bool VtStore::DeleteLabel(LabelId label) {
    return DeleteLabel(label, utils::TimeSpan());
  }



} // storage