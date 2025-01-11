//
// Created by marcob0020 on 11/3/24.
//

#ifndef VT_STORE_HPP
#define VT_STORE_HPP
#include <bit>
#include <utils/interval.hpp>

#include "comparers.hpp"
#include "edge_ref.hpp"
#include "id_types.hpp"
#include "property_value.hpp"
#include "utils/timespan.hpp"



namespace storage {
  namespace durability {
    class BaseEncoder;
    class BaseDecoder;
  }

  struct Vertex;
  class Storage;


  inline auto EdgeStoreType_Compare = [](const std::tuple<EdgeTypeId, Vertex *, EdgeRef> &lhs, const std::tuple<EdgeTypeId, Vertex *, EdgeRef> &rhs) -> int {

    return 0;

  };

class VtStore {
  static_assert(std::endian::native == std::endian::little, "PropertyStore supports only architectures using little-endian.");
public:
  VtStore();

  VtStore(const VtStore &) = delete;
  VtStore(VtStore &&other) noexcept;
  VtStore &operator=(const VtStore &) = delete;
  VtStore &operator=(VtStore &&other) noexcept;

  ~VtStore() = default;

  /// Returns the currently stored values for property `property` in between the utils::TimeSpan "vt". If the
  /// property doesn't exist a Null value is returned. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::valued_timeline<PropertyValue> GetProperty(PropertyId property, const utils::TimeSpan& vt) const;

  /// Checks whether the property `property` exists in the store in between the utils::TimeSpan "vt". The time
  /// complexity of this function is O(n).
  bool HasProperty(PropertyId property, const utils::TimeSpan& vt) const;

  /// Checks whether the property `property` exists in the store at all. The time
  /// complexity of this function is O(n).
  bool HasProperty(PropertyId property) const;

  /// Checks whether the property `property` is equal to the specified value
  /// `value` for all the filter in all the utils::TimeSpan "vt" . This function doesn't perform any memory allocations while
  /// performing the equality check. The time complexity of this function is
  /// O(n).
  bool IsPropertyEqual(PropertyId property, const PropertyValue &value, const utils::TimeSpan& vt) const;

  /// Returns all properties ids currently stored in the store. The time complexity
  /// of this function is O(n).
  /// @throw std::bad_alloc
  std::vector<PropertyId> Properties() const;

  /// Set a property value in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool SetProperty(PropertyId property, const PropertyValue &value, const utils::TimeSpan& vt);

  /// Init the timeline with the initial value in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool InitProperty(PropertyId property, const PropertyValue &value);

  /// Remove all properties and return `true` if any removal took place.
  /// `false` is returned if there were no properties to remove. The time
  /// complexity of this function is O(1).
  /// @throw std::bad_alloc
  bool ClearProperties();

  /// Remove all properties and return `true` if any removal took place.
  /// `false` is returned if there were no properties to remove. The time
  /// complexity of this function is O(1).
  /// @throw std::bad_alloc
  bool ClearProperties(utils::TimeSpan& vt);

  using EdgeStoreType = std::tuple<EdgeTypeId, Vertex *, EdgeRef>;

  /// Returns an interval which contains "true" for every vt in between the utils::TimeSpan 'vt' so that
  /// an outgoing edge 'edge' exists in that period. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::timeline GetOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt) const;

  /// Returns an interval which contains "true" for every vt in between the utils::TimeSpan 'vt' so that
  /// an ingoing edge 'edge' exists in that period. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::timeline GetIngoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt) const;

  /// Checks whether there is an outgoing edge `edge` in the store in between the utils::TimeSpan "vt". The time
  /// complexity of this function is O(n).
  bool HasOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt) const;

  /// Checks whether there is an ingoing edge `edge` in the store in between the utils::TimeSpan "vt". The time
  /// complexity of this function is O(n).
  bool HasIngoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt) const;

  /// Checks whether there is an outgoing edge `edge` in the store at all. The time
  /// complexity of this function is O(n).
  bool HasOutgoingEdge(EdgeStoreType edge) const;

  /// Checks whether there is an outgoing edge `edge` in the store at all. The time
  /// complexity of this function is O(n).
  bool HasIngoingEdge(EdgeStoreType edge) const;

  /// Returns all ingoing edges currently stored in the store. The time complexity
  /// of this function is O(n).
  /// @throw std::bad_alloc
  std::vector<EdgeStoreType> IngoingEdges() const;

  /// Returns all outgoing edges currently stored in the store. The time complexity
  /// of this function is O(n).
  /// @throw std::bad_alloc
  std::vector<EdgeStoreType> OutgoingEdges() const;

  /// Link an outgoing edge "edge" in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool SetOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt);

  /// Unlink an outgoing edge "edge" in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteOutgoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt);

  /// Link an ingoing edge "edge" in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool SetIngoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt);

  /// Unlink an outgoing edge "edge" in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteIngoingEdge(EdgeStoreType edge, const utils::TimeSpan& vt);

  /// Returns an interval which contains "true" for every vt in between the utils::TimeSpan 'vt' so that
  /// this graph object exists in that period. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::timeline GetObjectValidity(const utils::TimeSpan& vt) const;

  /// Mark the object as valid and existing in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool CreateObject(const utils::TimeSpan& vt);

  /// Mark the object as deleted in a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteObject(const utils::TimeSpan& vt);

  /// Mark the object as deleted from the start to the end of times. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteObject();

  /// Returns a bool "true" if this graph object exists at least onece
  ///  The time complexity of this function is O(1).
  bool IsValid() const;

  //// Returns an interval which contains "true" for every vt in between the utils::TimeSpan 'vt' so that
  /// the label "label" exists in that period. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::timeline GetLabel(LabelId label, const utils::TimeSpan& vt) const;

  /// Checks whether the label "label" exists in the store in between the utils::TimeSpan "vt". The time
  /// complexity of this function is O(n).
  bool HasLabel(LabelId label, const utils::TimeSpan& vt) const;

  /// Checks whether the label `label` exists in the store at all. The time
  /// complexity of this function is O(n).
  bool HasLabel(LabelId label) const;

  /// Returns all label ids currently stored in the store. The time complexity
  /// of this function is O(n).
  /// @throw std::bad_alloc
  std::vector<LabelId> Labels() const;

  /// Set a label a utils::TimeSpan "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool SetLabel(LabelId label, const utils::TimeSpan& vt);

  /// Remove the label and return `true` if any removal took place.
  /// `false` is returned if there were no utils::TimeSpan to remove. The time
  /// complexity of this function is O(1).
  /// @throw std::bad_alloc
  bool DeleteLabel(LabelId label);

  /// Remove the labels in a time span 'vt' and return `true` if any removal took place.
  /// `false` is returned if there were no properties to remove. The time
  /// complexity of this function is O(1).
  /// @throw std::bad_alloc
  bool DeleteLabel(LabelId label, const utils::TimeSpan& vt);

  void SerializeToWriter(durability::BaseEncoder* encoder) const;
  std::map<std::string,std::string> SerializeToStrings() const;

  void DeserializeIntoValidity(durability::BaseDecoder* decoder);
  void DeserializeIntoInEdges(durability::BaseDecoder* decoder, std::optional<EdgeStoreType> edge);
  void DeserializeIntoOutEdges(durability::BaseDecoder* decoder, std::optional<EdgeStoreType> edge);
  void DeserializeIntoProperty(durability::BaseDecoder* decoder, std::optional<PropertyId> property);

  using TimelineList = std::vector<utils::TimeSpan>;
  using ValuedTimeline = std::vector<std::pair<utils::TimeSpan, PropertyValue>>;


private:
    TimelineList lifetime_;
    std::map<LabelId, TimelineList> labels_;
    std::map<EdgeStoreType, TimelineList, EdgeStoreTypeComparer> ingoing_edges_;
    std::map<EdgeStoreType, TimelineList, EdgeStoreTypeComparer> outgoing_edges_;
    std::map<PropertyId, ValuedTimeline> properties_;
};



} // storage

#endif //VT_STORE_HPP
