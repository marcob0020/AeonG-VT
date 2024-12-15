//
// Created by marcob0020 on 10/22/24.
//

#include "interval.hpp"
#include "temporal_functions.hpp"

namespace utils {

  template<typename T>
  std::optional<typename valued_timeline<T>::Iterator> valued_timeline<T>::seek(valued_timeline<T>::Iterator start, VTDateTime vt) {
    auto it = start, prev = start;
    while (it != end()) {
      VTDateTime& span_start = (it->first.first);
      if (span_start == vt) {
        return it;
      }

      if (span_start > vt) {
        return prev;
      }
      prev = it;
    }
    return std::nullopt;
  }

  template<typename T>
  std::optional<typename valued_timeline<T>::ConstIterator> valued_timeline<T>::seek(valued_timeline<T>::ConstIterator start, VTDateTime vt) const {
    auto it = start, prev = start;
    while (it != end()) {
      VTDateTime& span_start = (it->first.first);
      if (span_start == vt) {
        return it;
      }

      if (span_start > vt) {
        return prev;
      }
      prev = it;
    }
    return std::nullopt;
  }

  template<typename T>
  void valued_timeline<T>::add(TimeSpan from_to, const T &value) {
    ValuedTimelineInsertion(from_to, false, _container_interval, value);
  }



  template<typename T>
  void valued_timeline<T>::remove(TimeSpan from_to) {
    if constexpr(is_bool<T>::value)
      TimelineInsertion(from_to, true, _container_interval);
    else
      ValuedTimelineInsertion(from_to, true, _container_interval, T());
  }

  template<typename T>
  void valued_timeline<T>::fill_voids(TimeSpan from_to, const T &value) {
    valued_timeline<T> inverse = this->split(from_to)->invert();

    for (auto it = inverse.begin(); it != inverse.end(); ++it) {
      _container_interval.emplace_front({it->first, value});
    }
  }

  template<typename T>
  bool valued_timeline<T>::covered(TimeSpan from_to) const {
    auto it_start = seek(begin(), from_to.first);
    auto it_end = seek(*it_start, from_to.second);

    if (it_start == std::nullopt)
      return false;

    if (it_start == it_end && it_end != end())
      return true;

    VTDateTime end_prev = from_to.first;

    for (auto it = *it_start; it != *it_end; it++) {
      if (it->first.first > from_to.second)
        return false;

      if (end_prev < it->first.first)
        return false;

      if (it->first.second >= from_to.second)
        return true;

      end_prev = it->first.second;
    }

    return false;

  }

  template<typename T>
  bool valued_timeline<T>::exists_outside(TimeSpan from_to) const {
    auto it_start = seek(begin(), from_to.first);

    if (it_start == std::nullopt || *it_start == end())
      return false;

    if ((*it_start) != begin() || (*it_start)->first.first < from_to.first)
      return true;

    auto it_end = seek(*it_start, from_to.second);

    if ((*it_end)-> first.first <= from_to.second)
      return (*it_end)->first.second > from_to.second;

    return (*it_end)++ != end();
  }

  template<typename T>
  T  valued_timeline<T>::get_single(TimeSpan from_to) const {
    auto it = seek(begin(), from_to.first);

    if (it == std::nullopt || *it == end())
      return T();

    if (from_to.included(it->first))
      return (*it)->second;

    return T();
  }

  template<typename T>
  T valued_timeline<T>::get_first(VTDateTime from) const {
    auto it = seek(begin(), from);

    if (it == std::nullopt || *it == end())
      return T();

    if ((*it)->first.first <= from)
      return (*it)->second;

    return T();
  }

  template<typename T>
  bool valued_timeline<T>::is_single(TimeSpan from_to) const {
    auto it = seek(begin(), from_to);

    if (it == std::nullopt || *it == end())
      return false;

    if (from_to.included((*it)->first))
      return true;

    return false;
  }

  template<typename T>
  bool valued_timeline<T>::has_any() {
    return !_container_interval.empty();
  }

  template<typename T>
  valued_timeline<T> valued_timeline<T>::split(TimeSpan from_to) const {
    valued_timeline<T> result(from_to);

    auto it_start = seek(begin(), from_to.first);
    if (it_start == std::nullopt || *it_start == end())
      return result;

    auto it_end = seek(*it_start, from_to.second);

    for (auto it = *it_start; it != it_end; it++) {
      if (it->first.first > from_to.second)
        break;

      auto it_intersect = from_to.intersect(it->first);

      if (it_intersect.valid())
        result.add(it_intersect, it->second);
    }

    return result;
  }

  template<typename T>
  timeline valued_timeline<T>::invert() const {
    timeline result (this->from_to);
    result.add(this->from_to);

    auto it_start = seek(begin(),from_to.first);

    for (auto it = *it_start; it != end(); it++) {
      auto it_intersect = it->first.intersect(this->from_to);

      if (it_intersect.valid()) {
        result.remove(it_intersect);
      }else break;
    }

    return result;
  }

  template<typename T>
  typename valued_timeline<T>::ConstIterator valued_timeline<T>::begin() const {
    return _container_interval.begin();
  }

  template<typename T>
  typename valued_timeline<T>::ConstIterator valued_timeline<T>::end() const {
    return _container_interval.end();
  }

  template<typename T>
  typename valued_timeline<T>::Iterator valued_timeline<T>::begin() {
    return _container_interval.begin();
  }

  template<typename T>
  typename valued_timeline<T>::Iterator valued_timeline<T>::end() {
    return _container_interval.end();
  }


  void timeline::add(TimeSpan from_to) {
    TimelineInsertion(from_to, false, _container_interval);
  }

  void timeline::remove(TimeSpan from_to) {
    TimelineInsertion(from_to, true, _container_interval);
  }

  bool timeline::covered() const {
    return TimelineCoverage(utils::TimeSpan(), _container_interval);
  }

  bool timeline::covered(TimeSpan from_to) const {
    return TimelineCoverage(from_to, _container_interval);
  }

  bool timeline::exists_outside(TimeSpan from_to) const {
    auto it_start = seek(begin(), from_to.first);

    if (it_start == std::nullopt || *it_start == end())
      return false;

    if ((*it_start) != begin() || (*it_start)->first < from_to.first)
      return true;

    auto it_end = seek(*it_start, from_to.second);

    if ((*it_end)-> first <= from_to.second)
      return (*it_end)->second > from_to.second;

    return (*it_end)++ != end();
  }

  bool timeline::get_single(TimeSpan from_to) const {
    auto it = seek(begin(), from_to.first);

    if (it == std::nullopt || *it == end())
      return false;

    if (from_to.included(**it))
      return true;

    return false;
  }

  bool timeline::get_first(VTDateTime from) const {
    auto it = seek(begin(), from);

    if (it == std::nullopt || *it == end())
      return false;

    if ((*it)->first <= from)
      return true;

    return false;
  }

  bool timeline::is_single(TimeSpan from_to) const {
    auto it = seek(begin(), from_to.first);

    if (it == std::nullopt || *it == end())
      return false;

    if (from_to.included((**it)))
      return true;

    return false;
  }


  timeline timeline::split(TimeSpan from_to) const {
    timeline result(from_to);

    auto it_start = seek(begin(), from_to.first);
    if (it_start == std::nullopt || *it_start == end())
      return result;

    auto it_end = seek(*it_start, from_to.second);

    for (auto it = *it_start; it != it_end; it++) {
      if (it->first > from_to.second)
        break;

      auto it_intersect = from_to.intersect(*it);

      if (it_intersect.valid())
        result.add(it_intersect);
    }

    return result;
  }

  timeline timeline::invert() const {
    timeline result (this->from_to);
    result.add(this->from_to);

    auto it_start = seek(begin(),from_to.first);

    for (auto it = *it_start; it != end(); it++) {
      auto it_intersect = it->intersect(this->from_to);

      if (it_intersect.valid()) {
        result.remove(it_intersect);
      }else break;
    }

    return result;
  }

  timeline::Iterator timeline::begin() {
    return _container_interval.begin();
  }

  timeline::Iterator timeline::end() {
    return _container_interval.end();
  }

  timeline::ConstIterator timeline::begin() const {
    return _container_interval.begin();
  }

  timeline::ConstIterator timeline::end() const {
    return _container_interval.end();
  }

  bool timeline::has_any() {
    return !_container_interval.empty();
  }

  std::optional<timeline::Iterator> timeline::seek(
    Iterator start, VTDateTime vt) {
    auto it = start, prev = start;
    while (it != end()) {
      VTDateTime& span_start = it->first;
      if (span_start == vt) {
        return it;
      }

      if (span_start > vt) {
        return prev;
      }
      prev = it;
    }
    return std::nullopt;
  }

  std::optional<timeline::ConstIterator> timeline::seek (timeline::ConstIterator start, VTDateTime vt) const {
    auto it = start, prev = start;
    while (it != end()) {
      VTDateTime span_start = it->first;
      if (span_start == vt) {
        return it;
      }

      if (span_start > vt) {
        return prev;
      }
      prev = it;
    }
    return std::nullopt;
  }


  // template<typename T, typename container>
  // void interval<T, container>::add(Period from_to, const T &value) {
  //   auto it_start = seek(begin(), from_to.first);
  //   auto it_end = seek(it_start, from_to.second);
  //
  //
  //
  //   if (it_start == it_end && it_start != end() && *it_start == value)
  //     return;
  //
  //   if (it_start == it_end && it_start != end() && *it_start != value) {
  //
  //   }
  //
  //
  //
  // }

}
