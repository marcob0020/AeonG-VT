//
// Created by marcob0020 on 11/3/24.
//

#include "vt_store.hpp"

#include <librdkafka/include/librdkafka/rdkafkacpp.h>
#include <query/temporal_filter.hpp>

namespace storage {

  VtStore::VtStore() {
    DeleteObject(TemporalPeriod());
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

  utils::interval<PropertyValue> VtStore::GetProperty(PropertyId property, const TemporalPeriod &vt) const {
    utils::interval<PropertyValue> result;

    auto it = properties_.find(property);

    if (it == properties_.end())
      return result;

    auto& timeline = it->second;

    for (const auto &kv : timeline) {
      if (vt.overlaps(kv.first)) {
        result.add(vt.intersect(kv.first).get_pair(), kv.second);
      }

      if (vt.second >= kv.first.second)
        break;
    }

    return result;
  }

  bool VtStore::HasProperty(PropertyId property, const TemporalPeriod& vt) const {
    auto it = properties_.find(property);

    if (it == properties_.end())
      return false;

    auto& timeline = it->second;

    for (const auto &kv : timeline) {
      if (vt.overlaps(kv.first)) {
        return true;
      }

      if (vt.second < kv.first.second)
        break;
    }

    return false;
  }

  bool VtStore::HasProperty(PropertyId property) const {
    auto it = properties_.find(property);

    return it != properties_.end() && !it->second.empty();
  }

  bool VtStore::IsPropertyEqual(PropertyId property, const PropertyValue &value, const TemporalPeriod &vt) const {
    auto it = properties_.find(property);

    if (it == properties_.end())
      return value.IsNull();

    auto& timeline = it->second;
    for (const auto &kv : timeline) {
      if (vt.included(kv.first)) {
        return kv.second == value;
      }

      if (vt.overlaps(kv.first) && kv.second != value) {
        return false;
      }

      if (vt.second < kv.first.second)
        break;
    }

    return value.IsNull();
  }

  std::vector<PropertyId> VtStore::Properties() const {
    std::vector<PropertyId> result;

    for (const auto &kv : properties_) {
      result.push_back(kv.first);
    }

    return result;
  }

  template<typename Container>
  void CombineIntoVector(const Container& timeline, const Container& v_new, typename Container::iterator itx) {
    auto it = itx == timeline.end() ? itx : itx + 1;
    timeline.insert(itx, v_new.begin(), v_new.end());
  }

  template<typename Container>
  void ReplaceIntoVector(const Container& timeline, const Container& v_new,typename Container::iterator delete_start, typename Container::iterator delete_end) {
    auto it_begin = timeline.begin();
    auto it_act = delete_start;

    if (it_act != timeline.end()) {
      it_act = timeline.erase(delete_start, delete_end);
    }else {
      it_act = delete_end;
    }

    timeline.insert(it_act, v_new.begin(), v_new.end());
  }

  template<typename Container>
  void DeleteFromVector(const Container& timeline, typename Container::iterator delete_start, typename Container::iterator delete_end) {
    auto it_act = delete_start;

    if (it_act != timeline.end()) {
      timeline.erase(delete_start, delete_end);
    }
  }

  bool VtStore::SetProperty(PropertyId property, const PropertyValue &value, const TemporalPeriod &vt) {
    auto it = properties_.find(property);
    bool write = false;

    if (it == properties_.end()) {
      std::vector<std::pair<TemporalPeriod,PropertyValue>> v;
      v.emplace_back(vt, value);
      properties_.emplace(property, std::move(v));

      return true;
    }

    auto& timeline = it->second;

    if (timeline.empty()) {
      timeline.emplace_back(vt, value);

      return true;
    }

    auto timeline_end = timeline.end();
    auto edit_start = timeline.end() , edit_end = timeline.end(), prev = timeline.begin(), delete_start = timeline.end(), delete_end = timeline.end();
    bool overlaps_start = false, overlaps_end = false, overlaps_same = false, found_start = false, found_end = false, found_replace = false, merges_start = false, merges_end = false, merges_same = false;

    bool vt_written = false, deleting = false;
    std::vector<std::pair<TemporalPeriod,PropertyValue>> v_new;
    v_new.reserve(2);

    for (auto itx = timeline.begin(); itx != timeline.end(); itx++) {
      if (vt.overlaps(itx->first)) {
        if (edit_start == timeline.begin()) {
          edit_start = itx;
        }
        if (vt.included(itx->first)) {
          if (itx->second == value) {
            return false;
          }

          write = true;
          PropertyValue val = itx->second;

          if (itx->first.first < vt.first) {
            itx->first.second = utils::VTDateTime::prev(vt.first);
            edit_start = itx;

            v_new.emplace_back(vt, value);
          }else {  // itx->first.first == vt.first
            itx->second = value;
          }

          if (itx->first.second > vt.second) {
            v_new.emplace_back(TemporalPeriod(utils::VTDateTime::next(vt.second),itx->first.second), val);
          }

          if (!v_new.empty())
            CombineIntoVector(timeline, v_new, itx);

          return true;

        }

        if (vt_written) {
          if (itx->first.second <= vt.second) {
            if (deleting)
              delete_end = itx;
            else {
              deleting = true;
              delete_start = itx;
              delete_end = itx;
            }
          }else {
            itx->first.first = utils::VTDateTime::next(vt.second);
            break;
          }
        }

        if (itx->first.first < vt.first) {
          if (itx->second == value) {
            vt_written = true;
            itx->first.second = vt.second;
          }else {
            itx->first.second = utils::VTDateTime::prev(vt.first);

            v_new.emplace_back(vt, value);
            vt_written = true;
          }
        }else if (itx->first.second > vt.second) {
          if (itx->second == value) {
            vt_written = true;
            itx->first.first = vt.first;
          }else {
            itx->first.first = utils::VTDateTime::next(vt.second);

            v_new.emplace_back(vt, value);
            vt_written = true;
          }
        }else {
          if (!vt_written) {
            itx->first = vt;
            itx->second = value;
          }
        }

      }

      if (itx->first.first > vt.second) {
        v_new.emplace_back(vt, value);
        vt_written = true;
        break;
      }
      prev = itx;
    }

    if (!vt_written) {
      v_new.emplace_back(vt, value);
    }

    ReplaceIntoVector(timeline, v_new, delete_start, delete_start == timeline.end() ? edit_start: delete_end );

    return write;
  }

  bool VtStore::ClearProperties() {
    bool empty = properties_.empty();

    properties_.clear();

    return !empty;
  }

  bool VtStore::ClearProperties(TemporalPeriod &vt) {

    bool edit = false;
    for (auto it = properties_.begin(); it != properties_.end(); it++) {
      if (it->second.empty())
        properties_.erase(it);

      else
        edit |= SetProperty(it->first,PropertyValue(),vt);
    }

    return edit;
  }

  template<typename T>
  utils::interval<bool> TimelineRetrieval(const TemporalPeriod &vt, const T& list) {
    utils::interval<bool> result;

    for (const auto &vtlist : list) {
      if (vt.included(vtlist)) {
        result.add(vt.get_pair(),true);
        break;
      }

      if (vt.overlaps(vtlist) ) {
        result.add(vt.intersect(vtlist).get_pair(),true);
      }

      if (vt.second < vtlist.second)
        break;
    }

    return result;
  }

  template<typename T>
  bool TimelineExistence(const TemporalPeriod &vt, const T& list) {
    for (const auto &vtlist : list) {
      if (vt.overlaps(vtlist)) {
        return true;
      }
    }

    return false;
  }

  template<typename T>
  bool TimelineInsertion(const TemporalPeriod& vt, bool inverse, const T& list) {
    //TODO ensure write is valued
    bool write = false;

    if (list.empty()) {
      if (inverse)
        return false;

      list.emplace_back(vt);
      return true;
    }

    if (vt.whole()) {
      if (inverse) {
        list.clear();
        return true;
      }

      list.clear();
      list.emplace_back(vt);
    }

    T v_new;
    v_new.reserve(2);

    bool vt_written = false, deleting = false;
    utils::VTDateTime end = utils::VTDateTime::min();

    auto delete_start = list.end(), delete_end = list.end(), edit_vt = list.end(), prev = list.begin();

    for (auto itx = list.begin(); itx != list.end(); itx++) {
      bool written;
      if (vt.overlaps(*itx) ) {
        if (!inverse) {
          if (!vt_written) {
            itx->first = utils::VTDateTime::less(vt.first,itx->first);
            itx->second = utils::VTDateTime::greater(vt.second,itx->second);
            vt_written = true;
            end = itx->second;
            edit_vt = itx;
          }else {
            if (end < itx->second) {
              end = itx->second;
              *edit_vt.second = end;
            }

            if (!deleting) {
              deleting = true;
              delete_start = itx;
            }
            delete_end = itx;
          }
        }else {
          if (itx->first < vt.first) {
            if (itx->second > vt.second) {
              v_new.emplace_back(utils::VTDateTime::next(vt.second), itx->second);
              break;
            }

            if (itx->second <= vt.second) {
              itx->second = utils::VTDateTime::prev(vt.first);
            }

          } else if (itx->first >= vt.first) {
            if (itx->second <= vt.second) {
              if (!deleting) {
                deleting = true;
                delete_start = itx;
              }
              delete_end = itx;
            }else {
              itx->second = utils::VTDateTime::next(vt.second);
              break;
            }
          }
        }

      }

      if (vt.second < itx.first) {
        if (vt_written) {
          if (itx.first == utils::VTDateTime::next(vt.second) && !inverse) {
            edit_vt->second = itx->second;

            if (!deleting) {
              delete_start = itx;
              deleting = true;
            }
            delete_end = itx;

          }
        }
        else if (!inverse) {
          v_new.emplace_back(vt);
          vt_written = true;
          edit_vt = prev;
        }
        break;
      }

      prev = itx;
    }

    if (!inverse && !vt_written) {
      v_new.emplace_back(vt);
      edit_vt = prev;
    }

    if (v_new.empty() && deleting) {
      DeleteFromVector(list, delete_start, delete_end);
    } else if (!v_new.empty() && !deleting) {
      CombineIntoVector(list, v_new, edit_vt);
    } else if (!v_new.empty() && deleting) {
      ReplaceIntoVector(list, v_new, delete_start, delete_end);
    }

    return write;

  }

  utils::interval<bool> VtStore::GetOutgoingEdge(EdgeStoreType edge, const TemporalPeriod &vt) const {
    const auto it = outgoing_edges_.find(edge);

    if (it == outgoing_edges_.end())
      return {};


    return TimelineRetrieval(vt, it->second);
  }

  utils::interval<bool> VtStore::GetIngoingEdge(EdgeStoreType edge, const TemporalPeriod &vt) const {
    const auto it = ingoing_edges_.find(edge);

    if (it == outgoing_edges_.end())
      return {};

    return TimelineRetrieval(vt, it->second);
  }

  bool VtStore::HasOutgoingEdge(EdgeStoreType edge, const TemporalPeriod& vt) const {
    const auto it = outgoing_edges_.find(edge);

    if (it == outgoing_edges_.end())
      return false;

    return TimelineExistence(vt, it->second);
  }

  bool VtStore::HasIngoingEdge(EdgeStoreType edge, const TemporalPeriod& vt) const {
    const auto it = ingoing_edges_.find(edge);

    if (it == ingoing_edges_.end())
      return false;

    return TimelineExistence(vt, it->second);
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

  bool VtStore::SetOutgoingEdge(EdgeStoreType edge, const TemporalPeriod &vt) {
    const auto it = outgoing_edges_.find(edge);

    if (it==outgoing_edges_.end()) {
      std::vector<TemporalPeriod> new_edges;
      new_edges.emplace_back(vt);
      outgoing_edges_[edge] = new_edges;

      return true;
    }

    return TimelineInsertion(vt, false, it->second);
  }

  bool VtStore::DeleteOutgoingEdge(EdgeStoreType edge, const TemporalPeriod &vt) {
    const auto it = outgoing_edges_.find(edge);

    if (it==outgoing_edges_.end())
      return true;

    return TimelineInsertion(vt, true, it->second);

  }


  bool VtStore::SetIngoingEdge(EdgeStoreType edge, const TemporalPeriod &vt) {
    const auto it = ingoing_edges_.find(edge);

    if (it==ingoing_edges_.end()) {
      std::vector<TemporalPeriod> new_edges;
      new_edges.emplace_back(vt);
      ingoing_edges_[edge] = new_edges;

      return true;
    }

    return TimelineInsertion(vt, false, it->second);
  }

  bool VtStore::DeleteIngoingEdge(EdgeStoreType edge, const TemporalPeriod &vt) {
    const auto it = ingoing_edges_.find(edge);

    if (it==ingoing_edges_.end())
      return true;

    return TimelineInsertion(vt, true, it->second);
  }


 utils::interval<bool> VtStore::GetObjectValidity(const TemporalPeriod &vt) const {
    return TimelineRetrieval(vt, lifetime_);
 }

 bool VtStore::CreateObject(const TemporalPeriod &vt) {
   return TimelineInsertion(vt, false, lifetime_);
 }

  bool VtStore::DeleteObject(const TemporalPeriod &vt) {
    return TimelineInsertion(vt, true, lifetime_);
  }

  bool VtStore::DeleteObject() {
    return TimelineInsertion(TemporalPeriod() , true, lifetime_);
  }



























} // storage