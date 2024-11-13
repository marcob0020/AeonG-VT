//
// Created by marcob0020 on 11/3/24.
//

#ifndef VT_STORE_HPP
#define VT_STORE_HPP
#include <bit>
#include <utils/interval.hpp>

#include "edge_ref.hpp"
#include "id_types.hpp"
#include "property_value.hpp"
#include "temporal_period.hpp"

namespace storage {
  struct Vertex;


  class VtStore {
  static_assert(std::endian::native == std::endian::little, "PropertyStore supports only architectures using little-endian.");
public:
  VtStore();

  VtStore(const VtStore &) = delete;
  VtStore(VtStore &&other) noexcept;
  VtStore &operator=(const VtStore &) = delete;
  VtStore &operator=(VtStore &&other) noexcept;

  ~VtStore() = default;

  /// Returns the currently stored values for property `property` in between the TemporalPeriod "vt". If the
  /// property doesn't exist a Null value is returned. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::interval<PropertyValue> GetProperty(PropertyId property, const TemporalPeriod& vt) const;

  /// Checks whether the property `property` exists in the store in between the TemporalPeriod "vt". The time
  /// complexity of this function is O(n).
  bool HasProperty(PropertyId property, const TemporalPeriod& vt) const;

  /// Checks whether the property `property` exists in the store at all. The time
  /// complexity of this function is O(n).
  bool HasProperty(PropertyId property) const;

  /// Checks whether the property `property` is equal to the specified value
  /// `value` for all the filter in all the TemporalPeriod "vt" . This function doesn't perform any memory allocations while
  /// performing the equality check. The time complexity of this function is
  /// O(n).
  bool IsPropertyEqual(PropertyId property, const PropertyValue &value, const TemporalPeriod& vt) const;

  /// Returns all properties ids currently stored in the store. The time complexity
  /// of this function is O(n).
  /// @throw std::bad_alloc
  std::vector<PropertyId> Properties() const;

  /// Set a property value in a TemporalPeriod "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool SetProperty(PropertyId property, const PropertyValue &value, const TemporalPeriod& vt);

  /// Remove all properties and return `true` if any removal took place.
  /// `false` is returned if there were no properties to remove. The time
  /// complexity of this function is O(1).
  /// @throw std::bad_alloc
  bool ClearProperties();

  /// Remove all properties and return `true` if any removal took place.
  /// `false` is returned if there were no properties to remove. The time
  /// complexity of this function is O(1).
  /// @throw std::bad_alloc
  bool ClearProperties(TemporalPeriod& vt);

  using EdgeStoreType = std::tuple<EdgeTypeId, Vertex *, EdgeRef>;

  /// Returns an interval which contains "true" for every vt in between the TemporalPeriod 'vt' so that
  /// an outgoing edge 'edge' exists in that period. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::interval<bool> GetOutgoingEdge(EdgeStoreType edge, const TemporalPeriod& vt) const;

  /// Returns an interval which contains "true" for every vt in between the TemporalPeriod 'vt' so that
  /// an ingoing edge 'edge' exists in that period. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::interval<bool> GetIngoingEdge(EdgeStoreType edge, const TemporalPeriod& vt) const;

  /// Checks whether there is an outgoing edge `edge` in the store in between the TemporalPeriod "vt". The time
  /// complexity of this function is O(n).
  bool HasOutgoingEdge(EdgeStoreType edge, const TemporalPeriod& vt) const;

  /// Checks whether there is an ingoing edge `edge` in the store in between the TemporalPeriod "vt". The time
  /// complexity of this function is O(n).
  bool HasIngoingEdge(EdgeStoreType edge, const TemporalPeriod& vt) const;

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

  /// Link an outgoing edge "edge" in a TemporalPeriod "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool SetOutgoingEdge(EdgeStoreType edge, const TemporalPeriod& vt);

  /// Unlink an outgoing edge "edge" in a TemporalPeriod "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteOutgoingEdge(EdgeStoreType edge, const TemporalPeriod& vt);

  /// Link an ingoing edge "edge" in a TemporalPeriod "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool SetIngoingEdge(EdgeStoreType edge, const TemporalPeriod& vt);

  /// Unlink an outgoing edge "edge" in a TemporalPeriod "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteIngoingEdge(EdgeStoreType edge, const TemporalPeriod& vt);

  /// Returns an interval which contains "true" for every vt in between the TemporalPeriod 'vt' so that
  /// this graph object exists in that period. The time complexity of
  /// this function is O(n).
  /// @throw std::bad_alloc
  utils::interval<bool> GetObjectValidity(const TemporalPeriod& vt) const;

  /// Mark the object as valid and existing in a TemporalPeriod "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool CreateObject(const TemporalPeriod& vt);

  /// Mark the object as deleted in a TemporalPeriod "vt" and return `true` if insertion took place. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteObject(const TemporalPeriod& vt);

  /// Mark the object as deleted from the start to the end of times. `false` is
  /// returned if even partial assignment took place. The time complexity of this function is
  /// O(n).
  /// @throw std::bad_alloc
  bool DeleteObject();

private:
    std::vector<TemporalPeriod> lifetime_;
    std::map<EdgeStoreType, std::vector<TemporalPeriod>> ingoing_edges_;
    std::map<EdgeStoreType, std::vector<TemporalPeriod>> outgoing_edges_;
    std::map<PropertyId, std::vector<std::pair<TemporalPeriod, PropertyValue>>> properties_;
};

} // storage

#endif //VT_STORE_HPP
