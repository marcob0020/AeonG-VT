//
// Created by marcob0020 on 9/28/24.
//

#pragma once

#include "utils/vt_temporal.hpp"

namespace utils{

struct TimeSpan {

  utils::VTDateTime first;
  utils::VTDateTime second;

  TimeSpan():first(utils::VTDateTime::min()),second(utils::VTDateTime::max()){}

  TimeSpan(const utils::VTDateTime& first, const utils::VTDateTime& second):first(first),second(second){}

  friend std::ostream &operator<<(std::ostream &os, const TimeSpan &ldt) {
    os << "{" << ldt.first << "," << ldt.second << "}";
    return os;
  }

  bool overlaps(const TimeSpan &other) const {
    return first < other.second && other.first < second;
  }

  bool overlaps_strict(const TimeSpan &other) const {
    return overlaps(other) && !equals(other) && in_between(other);
  }

  ///@returns true if this timespan is completely included in the other timespan
  ///Es. this       | | | | |#| | | | | |
  ///Es. other      | | |#|#|#|#|#| | | |
  bool included(const TimeSpan &other) const {
    return other.includes(*this);
  }

  ///@returns true if this timespan includes completely the other
  ///Es. this       | | |#|#|#|#|#| | | |
  ///Es. other      | | | | |#| | | | | |
  bool includes(const TimeSpan &other) const {
    return first <= other.first && second >= other.second;
  }

  ///@returns true if this timespan overlaps on start or on end with the other timespan
  bool in_between(const TimeSpan &other) const {
    return (first >= other.first && first <= other.second) || (second >= other.first && second <= other.second);
  }

  bool equals(const TimeSpan& other) const {
    return first == other.first && second == other.second;
  }

  bool whole() const {
    return first == utils::VTDateTime::min() && second == utils::VTDateTime::max();
  }

  bool valid() const {
    return first <= second;
  }

  TimeSpan intersect(const TimeSpan &other) const {
    return TimeSpan(std::max(first, other.first), std::min(other.second, second));
  }

  TimeSpan merges(const TimeSpan &other) const {
    return TimeSpan(std::min(first, other.first), std::max(second, other.second));
  }

  std::pair<utils::VTDateTime, utils::VTDateTime> get_pair() const {
    return std::make_pair(first, second);
  }

  bool operator == (const TimeSpan &other) const {
    return first == other.first && second == other.second;
  }

  bool operator != (const TimeSpan &other) const {
    return first != other.first || second != other.second;
  }
};

}


