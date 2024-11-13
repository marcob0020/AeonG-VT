//
// Created by marcob0020 on 9/28/24.
//

#pragma once

#include "utils/vt_temporal.hpp"

namespace storage{

struct TemporalPeriod {

  utils::VTDateTime first;
  utils::VTDateTime second;

  TemporalPeriod():first(utils::VTDateTime::min()),second(utils::VTDateTime::max()){}

  TemporalPeriod(const utils::VTDateTime& first, const utils::VTDateTime& second):first(first),second(second){}

  friend std::ostream &operator<<(std::ostream &os, const TemporalPeriod &ldt) {
    os << "{" << ldt.first << "," << ldt.second << "}";
    return os;
  }

  bool overlaps(const TemporalPeriod &other) const {
    return first <= other.second && other.first <= second;
  }

  bool overlaps_strict(const TemporalPeriod &other) const {
    return overlaps(other) && !equals(other) && in_between(other);
  }

  ///@returns true if this timespan is completely included in the other timespan
  ///Es. this       | | | | |#| | | | | |
  ///Es. other      | | |#|#|#|#|#| | | |
  bool included(const TemporalPeriod &other) const {
    return other.includes(*this);
  }

  ///@returns true if this timespan includes completely the other
  ///Es. this       | | |#|#|#|#|#| | | |
  ///Es. other      | | | | |#| | | | | |
  bool includes(const TemporalPeriod &other) const {
    return first <= other.first && second >= other.second;
  }

  ///@returns true if this timespan overlaps on start or on end with the other timespan
  bool in_between(const TemporalPeriod &other) const {
    return first >= other.first && first <= other.second || second >= other.first && second <= other.second;
  }

  bool equals(const TemporalPeriod& other) const {
    return first == other.first && second == other.second;
  }

  bool whole() const {
    return first == utils::VTDateTime::min() && second == utils::VTDateTime::max();
  }

  TemporalPeriod intersect(const TemporalPeriod &other) const {
    return TemporalPeriod(std::max(first, other.first), std::min(other.second, second));
  }

  TemporalPeriod merges(const TemporalPeriod &other) const {
    return TemporalPeriod(std::min(first, other.first), std::max(second, other.second));
  }

  std::pair<utils::VTDateTime, utils::VTDateTime> get_pair() const {
    return std::make_pair(first, second);
  }
};

}


