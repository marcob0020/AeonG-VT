//
// Created by marcob0020 on 11/30/24.
//
#pragma once
#include <utils/timespan.hpp>

#include "vt_temporal.hpp"
#include "interval.hpp"



namespace utils {


  template<typename Item>
  void CombineIntoVector(std::vector<Item>& timeline, const std::vector<Item>& v_new, typename std::vector<Item>::iterator itx) {
    // auto it = itx == timeline.end() ? itx : itx + 1;
    timeline.insert(itx, v_new.begin(), v_new.end());
  }

  template<typename Item>
  void CombineIntoVector(std::forward_list<Item>& timeline, const std::vector<Item>& v_new, typename std::forward_list<Item>::iterator itx) {
    // auto it = itx == timeline.end() ? itx : itx + 1;
    std::forward_list<Item> tmp;
    for (auto it = v_new.rbegin(); it != v_new.rend(); it++) {
      tmp.push_front(*it);
    }

    timeline.insert_after(itx, tmp.begin(), tmp.end());
  }

  template<typename Item>
  void ReplaceIntoVector(std::vector<Item>& timeline, const std::vector<Item>& v_new,typename std::vector<Item>::iterator delete_start, typename std::vector<Item>::iterator delete_end) {
    auto it_begin = timeline.begin();
    auto it_act = delete_start;

    if (it_act != timeline.end()) {
      it_act = timeline.erase(delete_start, delete_end);
    }else {
      it_act = delete_end;
    }

    timeline.insert(it_act, v_new.begin(), v_new.end());
  }

  template<typename Item>
  void ReplaceIntoVector(std::forward_list<Item>& timeline, const std::vector<Item>& v_new,typename std::forward_list<Item>::iterator delete_start, typename std::forward_list<Item>::iterator delete_end) {
    auto it_begin = timeline.begin();
    auto it_act = delete_start;
    auto it_prev = timeline.before_begin();

    for (auto it = it_begin; it != it_act; ++it) {
      it_prev = it;
    }

    if (it_prev != timeline.end()) {
      timeline.erase_after(it_prev, std::next(delete_end));
    }

    std::forward_list<Item> tmp;
    for (auto it = v_new.rbegin(); it != v_new.rend(); it++) {
      tmp.push_front(*it);
    }

    timeline.insert_after(it_prev, tmp.begin(), tmp.end());
  }

  template<typename Item>
  void DeleteFromVector(std::vector<Item>& timeline, typename std::vector<Item>::iterator delete_start, typename std::vector<Item>::iterator delete_end) {
    auto it_act = delete_start;

    if (it_act != timeline.end()) {
      timeline.erase(delete_start, delete_end);
    }
  }

  template<typename Item>
  void DeleteFromVector(std::forward_list<Item>& timeline, typename std::forward_list<Item>::iterator delete_start, typename std::forward_list<Item>::iterator delete_end) {
    auto it_act = delete_start;
    auto it_prev = timeline.before_begin();

    for (auto it = timeline.begin(); it != it_act; ++it) {
      it_prev = it;
    }

    if (it_prev != timeline.end()) {
      timeline.erase_after(it_prev, std::next(delete_end));
    }

  }

  template<typename Item>
  void InsertIntoVector(std::vector<Item>& timeline, const Item& item) {
    timeline.push_back(item);
  }

  template<typename Item>
  void InsertIntoVector(std::forward_list<Item>& timeline, const Item& item) {
    auto it_prev = timeline.before_begin();
    for (auto it = timeline.begin(); it != timeline.end(); ++it) {
      it_prev = it;
    }

    timeline.insert_after(it_prev, item);
  }

  template<typename T>
  bool TimelineInsertion(const utils::TimeSpan& vt, bool inverse, T& list) {
    //TODO ensure write is valued
    bool write = false;

    if (list.empty()) {
      if (inverse)
        return false;

      InsertIntoVector(list, vt);
      return true;
    }

    if (vt.whole()) {
      if (inverse) {
        list.clear();
        return true;
      }

      list.clear();
      InsertIntoVector(list, vt);
    }

    std::vector<typename T::value_type> v_new;
    v_new.reserve(2);

    bool vt_written = false, deleting = false;
    VTDateTime end = VTDateTime::min();

    auto delete_start = list.end(), delete_end = list.end(), edit_vt = list.end(), prev = list.begin();

    for (auto itx = list.begin(); itx != list.end(); itx++) {
      bool written;
      if (vt.overlaps(*itx) ) {
        if (!inverse) {
          if (!vt_written) {
            itx->first = VTDateTime::less(vt.first,itx->first);
            itx->second = VTDateTime::greater(vt.second,itx->second);
            vt_written = true;
            end = itx->second;
            edit_vt = itx;
          }else {
            if (end < itx->second) {
              end = itx->second;
              edit_vt->second = end;
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
              v_new.emplace_back(VTDateTime::next(vt.second), itx->second);
              break;
            }

            if (itx->second <= vt.second) {
              itx->second = VTDateTime::prev(vt.first);
            }

          } else if (itx->first >= vt.first) {
            if (itx->second <= vt.second) {
              if (!deleting) {
                deleting = true;
                delete_start = itx;
              }
              delete_end = itx;
            }else {
              itx->second = VTDateTime::next(vt.second);
              break;
            }
          }
        }

      }

      if (vt.second < itx->first) {
        if (vt_written) {
          if (itx->first == VTDateTime::next(vt.second) && !inverse) {
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

  template<typename T>
  timeline TimelineRetrieval(const TimeSpan &vt, const T& list) {
    timeline result(vt);

    for (const auto &vtlist : list) {
      if (vt.included(vtlist)) {
        result.add(vt);
        break;
      }

      if (vt.overlaps(vtlist) ) {
        result.add(vt.intersect(vtlist));
      }

      if (vt.second < vtlist.second)
        break;
    }

    return result;
  }

  template<typename T>
  bool TimelineExistence(const TimeSpan &vt, const T& list) {
    for (const auto &vtlist : list) {
      if (vt.overlaps(vtlist)) {
        return true;
      }
    }

    return false;
  }

  template<typename T>
  bool TimelineCoverage(const TimeSpan &vt, const T& list) {
    for (const auto &vtlist : list) {
      if (vt.included(vtlist)) {
        return true;
      }

      if (vt.second < vtlist.first)
        break;
    }
    return false;
  }

  template<typename T, typename V>
  bool ValuedTimelineInsertion(const TimeSpan& vt, bool inverse, T& list, const V& value) {
    bool write = false;

    auto& timeline = list;

    if (timeline.empty()) {
      InsertIntoVector(timeline, std::make_pair(vt, value));

      return true;
    }

    auto edit_start = timeline.end() , edit_end = timeline.end(), prev = timeline.begin(), delete_start = timeline.end(), delete_end = timeline.end();

    bool vt_written = false, deleting = false;
    std::vector<std::pair<TimeSpan,V>> v_new;
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
          V val = itx->second;

          if (itx->first.first < vt.first) {
            itx->first.second = VTDateTime::prev(vt.first);
            edit_start = itx;

            v_new.emplace_back(vt, value);
          }else {  // itx->first.first == vt.first
            itx->second = value;
          }

          if (itx->first.second > vt.second) {
            v_new.emplace_back(TimeSpan(VTDateTime::next(vt.second),itx->first.second), val);
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
            itx->first.first = VTDateTime::next(vt.second);
            break;
          }
        }

        if (itx->first.first < vt.first) {
          if (itx->second == value) {
            vt_written = true;
            itx->first.second = vt.second;
          }else {
            itx->first.second = VTDateTime::prev(vt.first);

            v_new.emplace_back(vt, value);
            vt_written = true;
          }
        }else if (itx->first.second > vt.second) {
          if (itx->second == value) {
            vt_written = true;
            itx->first.first = vt.first;
          }else {
            itx->first.first = VTDateTime::next(vt.second);

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

  template<typename T, typename V>
  bool ValuedTimelineEquals(const TimeSpan &vt, const T& list, const V& value) {
    auto& timeline = list;
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

    return false;
  }

  template<typename T, typename V>
  valued_timeline<V> ValuedTimelineRetrieval(const TimeSpan &vt, const T& list) {
    valued_timeline<V> result(vt);

    for (const auto &kv : list) {
      if (vt.overlaps(kv.first)) {
        result.add(vt.intersect(kv.first), kv.second);
      }

      if (vt.second >= kv.first.second)
        break;
    }

    return result;
  }

  template<typename T>
  bool ValuedTimelineExistence(const TimeSpan &vt, const T& list) {
    for (const auto &kv : list) {
      if (vt.overlaps(kv.first)) {
        return true;
      }

      if (vt.second < kv.first.second)
        break;
    }

    return false;
  }


}
