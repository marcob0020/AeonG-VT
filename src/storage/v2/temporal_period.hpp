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

  bool included(const TemporalPeriod &other) const {
    return first <= other.first && second <= other.second;
  }

  TemporalPeriod intersect(const TemporalPeriod &other) const {
    return TemporalPeriod(std::max(first, other.first), std::min(other.second, second));
  }

  TemporalPeriod merges(const TemporalPeriod &other) const {
    return TemporalPeriod(std::min(first, other.first), std::max(second, other.second));
  }
};

}


